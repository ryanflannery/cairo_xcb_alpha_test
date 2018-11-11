# cairo-xcb-alpha-test
sample app for me to test alpha blending in cairo+xcb on openbsd.
i was struggling to get true transparency working on the main background
window in [oxbar](https://github.com/ryanflannery/oxbar).

i build this poc to help me figure it out. basically, you need a compositing
window manager (like xcompmgr in base or compton from ports) running to make
transparency to the root window work.

much thanks to Uli Schlachter from the cairo mailing lists to helping me
figure out the root problem in:
https://lists.cairographics.org/archives/cairo/2018-September/028745.html
