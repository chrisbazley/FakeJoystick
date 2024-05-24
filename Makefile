# Project:   FakeJSMod


# Toolflags:
CCflags = -c -depend !Depend -IC: -throwback -zM -ff -zps1 -Ospace 
C++flags = -c -depend !Depend -IC: -throwback
Linkflags = -rmf -c++ -o $@ 
ObjAsmflags = -throwback -NoCache -depend !Depend
CMHGflags = -depend !Depend -throwback -IC: -32bit 
LibFileflags = -c -o $@
Squeezeflags = -o $@
ASMflags = -processor ARM2 -throwback -apcs 32 


# Final targets:
@.FakeJoystick:   @.o.FakeJoystickHdr @.o.FakeJoystick C:o.stubs @.o.errors 
        Link $(Linkflags) @.o.FakeJoystickHdr @.o.FakeJoystick C:o.stubs \
        @.o.errors 


# User-editable dependencies:


# Static dependencies:
@.o.FakeJoystickHdr:   @.cmhg.FakeJoystickHdr
        cmhg @.cmhg.FakeJoystickHdr -o @.o.FakeJoystickHdr
@.o.FakeJoystick:   @.c.FakeJoystick
        cc $(ccflags) -o @.o.FakeJoystick @.c.FakeJoystick 
@.o.errors:   @.a.errors
        ASM $(ASMFlags) -output @.o.errors @.a.errors


# Dynamic dependencies:
o.FakeJoystick:	c.FakeJoystick
o.FakeJoystick:	C:h.kernel
o.FakeJoystick:	C:h.swis
o.FakeJoystick:	h.FakeJoystickHdr
