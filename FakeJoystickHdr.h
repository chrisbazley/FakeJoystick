/*
 * Created by cmhg vsn 5.37 [16 Mar 2001]
 */

#ifndef __cmhg_FakeJoystickHdr_h
#define __cmhg_FakeJoystickHdr_h

#ifndef __kernel_h
#include "kernel.h"
#endif

#define CMHG_VERSION 537

#define Module_Title                     "Joystick"
#define Module_Help                      "FakeJoystick"
#define Module_VersionString             "2.00"
#define Module_VersionNumber             200
#ifndef Module_Date
#define Module_Date                      "11 Jun 2002"
#endif


/*
 * Initialisation code
 * ===================
 *
 * Return NULL if your initialisation succeeds; otherwise return a pointer
 * to an error block. cmd_tail points to the string of arguments with which
 * the module is invoked (may be "", and is control-terminated, not zero
 * terminated).
 * podule_base is 0 unless the code has been invoked from a podule.
 * pw is the 'R12' value established by module initialisation. You may
 * assume nothing about its value (in fact it points to some RMA space
 * claimed and used by the module veneers). All you may do is pass it back
 * for your module veneers via an intermediary such as SWI OS_CallEvery
 * (use _swix() to issue the SWI call).
 */
_kernel_oserror *FakeJoystick_initialise(const char *cmd_tail, int podule_base, void *pw);


/*
 * Finalisation code
 * =================
 *
 * Return NULL if your finalisation succeeds. Otherwise return a pointer to
 * an error block if your finalisation handler does not wish to die (e.g.
 * toolbox modules return a 'Task(s) active' error).
 * fatal, podule and pw are the values of R10, R11 and R12 (respectively)
 * on entry to the finalisation code.
 */
_kernel_oserror *FakeJoystick_finalise(int fatal, int podule, void *pw);


/*
 * Command handler
 * ===============
 *
 * If cmd_no identifies a command, then arg_string gives the command tail
 * (which you may not overwrite), and argc is the number of parameters.
 * Return NULL if the command has been successfully handled; otherwise
 * return a pointer to an error block describing the failure (in this
 * case, the veneer code will set the 'V' bit).
 *
 * If cmd_no identifies a *Help entry, then arg_string denotes a buffer
 * that you can assemble your output into. cmd_handler must return
 * NULL, an error pointer or help_PRINT_BUFFER (if help_PRINT_BUFFER)
 * is returned, the zero-terminated buffer will be printed).
 *
 * If cmd_no identifies a *Configure option, then arg_string gives the
 * command tail, and argc the number of parameters. Return NULL, an error
 * pointer, or one of the four special values defined below. If arg_string
 * is set to arg_CONFIGURE_SYNTAX, the user has typed *Configure with no
 * parameter; simply print your syntax string. If arg_string is set to
 * arg_STATUS, print your current configured status. Otherwise use
 * arg_string and argc to set the *Configure option.
 *
 * pw is the private word pointer ('R12') value passed into the entry
 * veneer
 */
#define help_PRINT_BUFFER         ((_kernel_oserror *) arg_string)
#define arg_CONFIGURE_SYNTAX      ((char *) 0)
#define arg_STATUS                ((char *) 1)
#define configure_BAD_OPTION      ((_kernel_oserror *) -1)
#define configure_NUMBER_NEEDED   ((_kernel_oserror *) 1)
#define configure_TOO_LARGE       ((_kernel_oserror *) 2)
#define configure_TOO_MANY_PARAMS ((_kernel_oserror *) 3)

#define CMD_FakeJSType                  0

_kernel_oserror *cmd_handler(const char *arg_string, int argc, int cmd_no, void *pw);


/*
 * SWI handler code
 * ================
 *
 * swi_offset contains the offset of the SWI into your SWI chunk.
 * r points to the registers passed to the SWI.
 *
 * Return NULL if the SWI is handled successfully; otherwise return
 * a pointer to an error block which describes the error.
 * The veneer code sets the 'V' bit if the returned value is non-NULL.
 * The special value error_BAD_SWI may be returned if you do not
 * implement a SWI; the veneer will arrange for the appropriate
 * standard internationalised error 'SWI value out of range for
 * module Joystick' to be returned.
 * The handler may update any of its input registers (R0-R9).
 * pw is the private word pointer ('R12') value passed into the
 * SWI handler entry veneer.
 */
#define Joystick_00                     0x043f40
#ifndef Joystick_Read
#define Joystick_Read                   0x043f40
#define Joystick_CalibrateTopRight      0x043f41
#define Joystick_CalibrateBottomLeft    0x043f42
#endif

#define error_BAD_SWI ((_kernel_oserror *) -1)

_kernel_oserror *FakeJoystick_swihandler(int swi_offset, _kernel_swi_regs *r, void *pw);


/*
 * Generic veneers
 * ===============
 *
 * This is the name of the generic entry veneer compiled by CMHG.
 * Use this name as an argument to, for example, SWI OS_CallEvery
 * or OS_AddCallBack.
 *
 * These veneers ensure that your handlers preserve R0-R11
 * and the processor flags (unless you return an error pointer.
 * The veneer can be entered in either IRQ or SVC mode. R12 and
 * R14 are corrupted.
 */
extern void callevery_veneer(void);

/*
 * This is the handler function that the veneer declared above
 * calls.
 *
 * For a standard exit, return NULL. For handlers that can return an
 * error, return an error block pointer, and the veneer will set the
 * 'V' bit, and set R0 to the error pointer.
 *
 * 'r' points to a vector of words containing the values of R0-R9 on
 * entry to the veneer. If r is updated, the updated values will be
 * loaded into R0-R9 on return from the handler.
 *
 * pw is the private word pointer ('R12') value with which the
 * entry veneer is called.
 */
_kernel_oserror *callevery_handler(_kernel_swi_regs *r, void *pw);


/*
 * Event handler
 * =============
 *
 * This is the name of the event handler entry veneer compiled by CMHG.
 * Use this name as an argument to, for example, SWI OS_Claim, in
 * order to attach your handler to EventV.
 */
extern void event_veneer(void);

/*
 * This is the handler function you must write to handle the event for
 * which event_veneer is the veneer function.
 *
 * Return 0 if you wish to claim the event.
 * Return 1 if you do not wish to claim the event.
 *
 * 'r' points to a vector of words containing the values of R0-R9 on
 * entry to the veneer. If r is updated, the updated values will be
 * loaded into R0-R9 on return from the handler.
 *
 * pw is the private word pointer ('R12') value with which the event
 * entry veneer is called.
 */
int event_handler(_kernel_swi_regs *r, void *pw);

#endif
