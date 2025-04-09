EA Compression Tool by Raynê Games

Tool to compress/decompress the files with JDLZ and HUFF compression used in
many EA games. The tool supports compress files to JDLZ and also HUFF. The
HUFF reach a compression ratio better than JDLZ.

Example of usage to decompress a file:
ea_compression_tool.exe -d infile outfile

infile= name of the file that will be decompressed
outfile= name of the file that will be contain the decompressed data

---------------------------------------------------------------------
Example of usage to compress a file using the HUFF compression:
ea_compression_tool.exe -c HUFF infile outfile

Compress using the HUFF compression the infile data and save the
compressed data to outfile

---------------------------------------------------------------------
Example of usage to compress a file using the JDLZ compression:
ea_compression_tool.exe -c JDLZ infile outfile

Compress using the JDLZ compression the infile data and save the
compressed data to outfile

---------------------------------------------------------------------
Use the -h option to help
ea_compression_tool.exe -h
