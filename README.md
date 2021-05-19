# pitcherpatcher

This is a tool for installing OpenFirmware nvramrcs on Gazelle and PowerSurge based machines.  This is especially useful for fiddling with the PCI device tree on Twentieth Anniversary Macs.

# how to build it

I use CodeWarrior Pro 6.  Other C compilers that can do inline PowerPC ASM will almost certainly do.

1. Create a resource file for the project in ResEdit (pitcher.rsrc).  This should contain a single NVRM resource, id=128, containing the Forth code you wish to install.
2. Build project, making sure the NVRM resource gets copied to the resultant application.
3. Try it out.  If it crashes hard, your nvramrc is probably too long; shorten it and try again.

# should I use it?

This *has* been tested.  By multiple people, even.  On different computers.  But it is ultimately a tool for inserting arbitrary Forth code early into the boot sequence; if this isn't something you know you want, you probably don't.

# credits

This is *heavily* based on the [ftp://ftp.netbsd.org/pub/NetBSD/arch/macppc/macos-utils/bootvars/]('Boot Variables') application, which was released under the GPL2.  In fact it's really just a stripped-down UI onto the same code.  Credits are in the source files involved; but in general, anything that works about this should be credited to the authors of that tool.  Misbehaviours can be my fault.
