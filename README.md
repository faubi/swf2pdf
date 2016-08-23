swf2pdf
=

This is a simple command-line program that takes one or more .swf files (Flash
animations) and renders the first frame of each into a pdf document (one per
page). Names of swf files to load can be supplied either as arguments or piped
to stdin.

This depends on the [swfdec-0.8](https://swfdec.freedesktop.org/wiki/) and
[cairo](https://www.cairographics.org/) libraries to do the heavy lifting, so
those must be installed for this to run.

Examples
-

* Render an swf file to output.pdf

    `swf2pdf myflash.swf`

* Specify a different name for the output file

    `swf2pdf myflash.swf -o mypdf.pdf`

* Render several swfs together

    `swf2pdf page1.swf page2.swf page3.swf -o pages.pdf`

* Supply swf files to stdin

    `cat swf-files.txt | swf2pdf`

