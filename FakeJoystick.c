/*
 *  FakeJoystick - joystick emulation module
 *  Copyright (C) 2002  Chris Bazley
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* ANSI headers */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

/* Acorn headers */
#include "kernel.h"
#include "swis.h"

/* CMHG header */
#include "FakeJoystickHdr.h"

/* OS_Byte routines */
#define OSB_ENABLEEVENT  14
#define OSB_DISABLEEVENT 13

/* Vector numbers */
#define VECTOR_EVENTV    16

/* Event numbers */
#define EVENT_KEYTRANS   11

/* Internal key numbers */
#define KEY_KP4     72  /* left */
#define KEY_KP6     74  /* right */
#define KEY_KP5     73  /* centre */
#define KEY_KP8     56  /* up */
#define KEY_KP2     91  /* down */
#define KEY_KPENTER 103 /* fire 1 */
#define KEY_KPPLUS  75  /* fire 2 */

/* Imaginary joystick state */
static signed char x_axis,y_axis;
static unsigned char buttons; /* bit field */

static char mode; /* type of emulation */
#define MODE_SWITCHED 0
#define MODE_ANALOGUE 1
#define MODE_DAMPED   2

static bool fake_calibrate_TR; /* waiting for Joystick_CalibrateBottomLeft? */
static bool fake_calibrate_BL; /* waiting for Joystick_CalibrateTopRight? */

static bool left, right, up, down; /* keys pressed state */

static signed int damp_x, damp_y; /* for damping algorithm */ 

extern _kernel_oserror bad_reason, error_no_mem, error_analogue, error_calib, FakeJSType_syntax; /* error blocks, assembled separately */

/* Convert supplied string to lower case */
#define lowercase(input) \
 { \
   char *string = input; \
   while(*string >= ' ') { \
     *string=(char)tolower((int)*string); \
     string++; \
   } \
 }

/* ----------------------------------------------------------------------- */

_kernel_oserror *FakeJoystick_initialise(const char *cmd_tail, int podule_base, void *pw)
{
  
  /* Reset imaginary joystick state */
  x_axis = 0;
  y_axis = 0;
  buttons = 0;
  mode = MODE_SWITCHED;
  fake_calibrate_BL = false;
  fake_calibrate_TR = false;
  damp_x = 0;
  damp_y = 0;
  
  /* Enable key transition event */
  if(_kernel_osbyte(OSB_ENABLEEVENT,EVENT_KEYTRANS,0) == _kernel_ERROR)
    return _kernel_last_oserror(); /* fail */

  /* Install event routine  */
  {
    _kernel_oserror *initerror;
    _kernel_swi_regs regs;
    regs.r[0] = VECTOR_EVENTV;
    regs.r[1] = (int)&event_veneer;
    regs.r[2] = (int)pw;
    initerror = _kernel_swi(OS_Claim, &regs, &regs);
    if(initerror != NULL) {
      _kernel_osbyte(OSB_DISABLEEVENT,EVENT_KEYTRANS,0); /* Refuse to live if we can't claim event vector */
      return initerror; /* fail */
    }
  }
  return NULL; /* success */
}

/* ----------------------------------------------------------------------- */

_kernel_oserror *cmd_handler(const char *arg_string, int argc, int cmd_no, void *pw)
{
  #define MAXARGS 1
  char *writeable_args;
  char *arg_ptrs[MAXARGS];
      
  /* Don't think we can get spurious commands, but just in case.... */
  /*printf("cmd_no: %d\nargc: %d\n",cmd_no,argc);*/
  if(cmd_no != CMD_FakeJSType)
    return NULL; /* success */

  {
    /* Make writable version of arg_string */
    int len = 0;
    /*printf("arg_string: '");*/
    while(arg_string[len] >= ' ') {
      /*printf("%c",arg_string[len]);*/
      len++;
    }
    /*printf("'\nlen: %d\n",len);*/

    writeable_args = malloc(len + 1);
    if (writeable_args == NULL)
      return &error_no_mem; /* fail */
    memcpy(writeable_args, arg_string, len + 1);
   
    /* Split up writeable_args into arg_ptrs */
    {
      int i, argcount = 0;
      for (i = 0; i < len; i++) /* Scan command tail... */
      {
        while (writeable_args[i] == ' ')  /* strip leading spaces */
          i++;
        arg_ptrs[argcount] = writeable_args + i;  /* record start of argument */
        while (i < len && writeable_args[i] != ' ')
          i++;
        writeable_args[i] = NULL;             /* zero terminate argument */
        /*printf("arg_ptrs[%d]: %s\n",argcount,arg_ptrs[argcount]);*/
        argcount++;
      }
    }
  }

  {
    /* FakeJSType [analogue|switched|damped] */
    _kernel_oserror *cmd_error = NULL;

    if(argc > 0) {
      /* set emulation type */
      lowercase(arg_ptrs[0]);
      if(strcmp(arg_ptrs[0], "switched") == 0) {
        if(mode != MODE_SWITCHED) {
          /* Remove OS_CallEvery routine */
          _kernel_swi_regs regs;
          regs.r[0] = (int)callevery_veneer;
          regs.r[1] = (int)pw;
          cmd_error = _kernel_swi(OS_RemoveTickerEvent, &regs,&regs);
          if(cmd_error == NULL)
            mode = MODE_SWITCHED;
        }
      } else {
        if(strcmp(arg_ptrs[0], "analogue") == 0) {
          if(mode == MODE_SWITCHED) {
            /* Attach OS_CallEvery routine */
            _kernel_swi_regs regs;
            regs.r[0] = (int)4; /* every 4 cs */
            regs.r[1] = (int)callevery_veneer;
            regs.r[2] = (int)pw;
            cmd_error = _kernel_swi(OS_CallEvery, &regs, &regs);
          }
          if(cmd_error == NULL)
            mode = MODE_ANALOGUE;
        } else {
          if(strcmp(arg_ptrs[0], "damped") == 0) {
            if(mode == MODE_SWITCHED) {
              /* Attach OS_CallEvery routine */
              _kernel_swi_regs regs;
              regs.r[0] = (int)4; /* every 4 cs */
              regs.r[1] = (int)callevery_veneer;
              regs.r[2] = (int)pw;
              cmd_error = _kernel_swi(OS_CallEvery, &regs, &regs);
            }
            if(cmd_error == NULL) {
              mode = MODE_DAMPED;
              damp_x = 0;
              damp_y = 0;
            }
          } else
            cmd_error = &FakeJSType_syntax;
        }
      }
      if(cmd_error == NULL) {
        /* Changing emulation type (reset) */
        x_axis = 0;
        y_axis = 0;
        fake_calibrate_BL = false;
        fake_calibrate_TR = false;
      }
    }
    else {
      /* display current setting */
      printf("Joystick emulation: ");
      switch(mode) {
      
        case MODE_SWITCHED:
          printf("Switched\n");
          break;
          
        case MODE_ANALOGUE:
          printf("Analogue\n");
          break;
          
        case MODE_DAMPED:
          printf("Damped\n");
          break;
      }
    }
    free(writeable_args);
    return cmd_error;
  }
}

/* ----------------------------------------------------------------------- */

_kernel_oserror *FakeJoystick_swihandler(int swi_no, _kernel_swi_regs *r, void *private_word)
{
  switch(swi_no) {
  
    case 0: /* Joystick_Read */
      if(fake_calibrate_TR || fake_calibrate_BL)
        return &error_calib; /* fail */
      {
        char stick_num = r->r[0] & 0xff;
        char reason_code = (r->r[0] & 0xff00) >> 8;
        if(stick_num == 0) {
          /* first joystick is emulated */
          switch(reason_code) {

            case 0:
              /* Read 8-bit state of an analogue or switched joystick*/
              if(stick_num == 0) {
                /* first joystick is emulated */
                if(mode == MODE_DAMPED)
                  r->r[0] = ((damp_y>>10) & 0xff) | (((damp_x>>10) & 0xff) << 8) | (buttons << 16);
                else
                  r->r[0] = (y_axis & 0xff) | ((x_axis & 0xff) << 8) | (buttons << 16);
              }
              else {
                /* other joysticks aren't */
                r->r[0] = 0; /* 8-bit centred, nothing pressed */
              }
              break;

            case 1:
              /* Read 16-bit state of an analogue joystick*/
              if(mode == MODE_SWITCHED)
                return &error_analogue; /* Analogue sticks only */  
              if(stick_num == 0) {
                /* first joystick is emulated */
                signed int x_16, y_16;
                if(mode == MODE_DAMPED) {
                  x_16 = 0x7fff + (damp_x >> 2);
                  y_16 = 0x7fff + (damp_y >> 2);
                }
                else {
                  x_16 = 0x7fff + ((signed int)x_axis << 8);
                  y_16 = 0x7fff + ((signed int)y_axis << 8);
                }
                /* (actual range is only 255-65279 rather than 0-65535) */
                r->r[0] = (y_16 & 0xffff) | ((x_16 & 0xffff) << 16);
                r->r[1] = buttons; /* switch state */
              }
              else {
                /* other joysticks aren't */
                r->r[0] = 0x7fff7fff; /* 16-bit centre position */
                r->r[1] = 0; /* switch state */
              }
              break;

            default:
              /* Unknown reason code! */
              return &bad_reason; /* fail */
          }
        }
      }
      return NULL; /* success */
      
    case 1: /* Joystick_CalibrateTopRight */
      if(mode == MODE_SWITCHED)
        return &error_analogue; /* Analogue sticks only */
      else {
        if(!fake_calibrate_BL)
          fake_calibrate_TR = true; /* calibration incomplete */
        else
          fake_calibrate_BL = false; /* calibration complete */
        return NULL; /* success */
      }
      
    case 2: /* Joystick_CalibrateBottomLeft */
      if(mode == MODE_SWITCHED)
        return &error_analogue; /* Analogue sticks only */
      else {
        if(!fake_calibrate_TR)
          fake_calibrate_BL = true; /* calibration incomplete */
        else
          fake_calibrate_TR = false; /* calibration complete */
        return NULL; /* success */
      }
      
    default:
      return error_BAD_SWI; /* fail */
  }
}

/* ----------------------------------------------------------------------- */

int event_handler(_kernel_swi_regs *r, void *pw)
{
  /* (no need to check event number, as CMHG veneer filters events for us) */
  
  switch(r->r[2]) {
    /* Check fire buttons */
    case KEY_KPENTER:
      if(r->r[1]) {
        /* press */
        buttons |= 1u<<0;
      }
      else {
        /* release */
        buttons &= ~(1u<<0);
      }
      break;
 
    case KEY_KPPLUS:
      if(r->r[1]) {
        /* press */
        buttons |= 1u<<1;
      }
      else {
        /* release */
        buttons &= ~(1u<<1);
      }
      break;
  }

  if(mode == MODE_SWITCHED) {
    /* emulate switched stick - which key is it? */
    switch(r->r[2]) {
      case KEY_KP4:
        if(r->r[1]) {
          /* press */
          x_axis = -64;
        }
        else {
          /* release */
          if(x_axis < 0)
            x_axis = 0;
        }
        break;
        
      case KEY_KP6:
        if(r->r[1]) {
          /* press */
          x_axis = 64;
        }
        else {
          /* release */
          if(x_axis > 0)
            x_axis = 0;
        }
        break;
        
      case KEY_KP8:
        if(r->r[1]) {
          /* press */
          y_axis = 64;
        }
        else {
          /* release */
          if(y_axis > 0)
            y_axis = 0;
        }
        break;
        
      case KEY_KP2:
        if(r->r[1]) {
          /* press */
          y_axis = -64;
        }
        else {
          /* release */
          if(y_axis < 0)
            y_axis = 0;
        }
        break;
        
      case KEY_KP5:
        if(r->r[1]) {
          /* press */
          x_axis = 0;
          y_axis = 0;
        }
        break;
      }
  } else {
    /* emulate analogue stick - which key is it? */
    switch(r->r[2]) {
      case KEY_KP4:
        left = r->r[1];
        break;
        
      case KEY_KP6:
        right = r->r[1];
        break;
        
      case KEY_KP8:
        up = r->r[1];
        break;
        
      case KEY_KP2:
        down = r->r[1];
        break;
        
      case KEY_KP5:
        if(r->r[1]) {
          /* press */
          x_axis = 0;
          y_axis = 0;
          damp_x = 0;
          damp_y = 0;
        }
        break;
    }
  }
  return 1;  /* pass event on to next claimant */
}

/* ----------------------------------------------------------------------- */

_kernel_oserror *callevery_handler(_kernel_swi_regs *r, void *pw)
{
  /* Called every 4 cs (25 times a second) */
  if(mode == MODE_DAMPED) {
  
    /* Gradual decay function */
    damp_x = damp_x - (damp_x>>4) - (damp_x>>5);
    damp_y = damp_y - (damp_y>>4) - (damp_y>>5);
    
    /* move stick according to keys */
    if(left) {
      damp_x -= 15<<10;
      if(damp_x >= 0)
        damp_x -= damp_x>>2;
      if(damp_x < -127<<10)
        damp_x = -127<<10;
    }
    if(right) {
      damp_x += 15<<10;
      if(damp_x < 0)
        damp_x -= damp_x>>2;
      if(damp_x > 127<<10)
        damp_x = 127<<10;
    }
    if(up) {
      damp_y += 15<<10;
      if(damp_y < 0)
        damp_y -= damp_x>>2;
      if(damp_y > 127<<10)
        damp_y = 127<<10;
    }
    if(down) {
      damp_y -= 15<<10;
      if(damp_y >= 0)
        damp_y -= damp_y>>2;
      if(damp_y < -127<<10)
        damp_y = -127<<10;
    }
  } else {
    /* move stick according to keys */
    if(left) {
      if(x_axis > -123)
        x_axis -= 5;
      else
        x_axis = -127;
    }
    if(right) {
      if(x_axis < 123)
        x_axis += 5;
      else
        x_axis = 127;
    }
    if(up) {
      if(y_axis < 123)
        y_axis += 5;
      else
        y_axis = 127;
    }
    if(down) {
      if(y_axis > -123)
        y_axis -= 5;
      else
        y_axis = -127;
    }
    /* Should traverse full range in 2 seconds (254/5 = 50) */
  }
  return NULL; /* success */
}

/* ----------------------------------------------------------------------- */

_kernel_oserror *FakeJoystick_finalise(int fatal, int podule, void *pw)
{
  _kernel_swi_regs regs;
  _kernel_oserror *err;
  
  /* Disable key transition event */
  if(_kernel_osbyte(OSB_DISABLEEVENT,EVENT_KEYTRANS,0)==_kernel_ERROR)
     return _kernel_last_oserror(); /* fail */

  /* Remove event handler */
  regs.r[0] = VECTOR_EVENTV;
  regs.r[1] = (int)&event_veneer;
  regs.r[2] = (int)pw;
  err = _kernel_swi(OS_Release, &regs, &regs);
  if(err != NULL)
    return err; /* fail */

  if(mode != MODE_SWITCHED) {
    /* Remove OS_CallEvery routine */
    regs.r[0] = (int)callevery_veneer;
    regs.r[1] = (int)pw;
    return _kernel_swi(OS_RemoveTickerEvent, &regs, &regs);
  }
  else
    return NULL; /* success */
}
