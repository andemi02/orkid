At one time or another Orkid has been ported to the PS2, NintendoDS, Wii, XBox360, Osx, Linux and Windows.

Unfortunately, with my job and family, I currently only have the bandwidth to maintain the Linux and Osx builds.
The Windows/D3D9 driver is still present in the source tree though it has likely fallen out of date. That said, it would not take too much effort to get it back in working condition.

If you want to minimize any pain, right now I would recommend Linux (Ubuntu 12.04LTS x86/64). The MacOsx (lion+) build needs a little work as it fails to build the loki library, my temporary solution for this will be to disable the tozkit dependency.

I do not currently test on Intel gfx chips. If you have an NVidia or AMD/ATI card that should be fine. I also recommend proprietary drivers over the open source ones. Open source is great and all, but I find the open source video drivers are still not up to par with their proprietary counterparts.

In general building will require a bunch of dependencies which are not included. There is a script included that automates the downloading, building and installation of these dependencies. Some of these dependencies include the 3delight renderer, cortex-vfx, alembic, Open Shading Language, QT5.0.1, etc.. Some of these will eventually be used for offline production quality rendering of content generated with Orkid. Note that 3Delight is free (even for commercial work), though it will be limited to 2 cores for rendering. If you want full renderman acceleration you will need to buy a license from http://www.3delight.com

To build on Ubuntu12.04 LTS x86/64
==================================
* clone it, cd into repo 
* make env (this will setup build environment on your local shell only. just "exit" to unset this environment)
* make get (to get dependencies, this can take a while)
* make pristine (to clear out the dependency build folders)
* make toz (to build dependencies, this can take a longer while, and will require you to press return when it gets to building alembic)
* make (to build orkid itself)

everything will be built/installed into the <repo_root>/stage folder.
the stage/bin and stage/lib paths were added to your environment variables already when you did a 'make env'.

To run
======
* symlink data->ork.data , Once I get an installer made, this step will become unnecessary. 
* run ork.tool.test.ix.release (from the repo root folder). It is in your path already, so just type ork.[tab tab] and see which orkid executables are present.


