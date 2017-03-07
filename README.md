# Parallel Data Streams (PDS) tool

Implemented by Nooshin Eghbal, Hamidreza Anvari, and others (see CREDITS)

Ph.D. supervisor:  Paul Lu

Copyright 2017 by Nooshin Eghbal, Hamidreza Anvari, Paul Lu.

Dept. of Computing Science, University of Alberta

Released under the GPL 3.0 licence.

Note:  This software is an Alpha release.  It works well enough for
us to experiment with it and produce real performance results on
both emulated WANs (using netem/tc) and a real WAN (between the
University of Alberta and Sherbrooke University).

Please see the draft paper in the repository (Docs) for more information.

Known issues, to be fixed:

1.  Testing on systems other than Linux.
2.  Unit tests required
3.  Command-line compatibility with Secure Shell/OpenSSH
4.  Lots of code clean-up.  Lots.  We know.  Help please.
5.  Documentation.

Known errors/mesages:

1.  After an rsync, you might see the error message:

“rsync error: sibling process crashed (code 15) at main.c(1165) [sender=3.1.1]”

However, the file has been transferred successfully.
