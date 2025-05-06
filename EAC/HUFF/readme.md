EA Compression Tool by Raynê Games

Tool to compress/decompress the files with JDLZ, HUFF, REFPACK and BTREE
compression used in many EA games. The tool supports compress files to JDLZ,
HUFF, REFPACK and BTREE. The HUFF reach a compression ratio better than JDLZ.

Example of usage to decompress a file:
ea_compression_tool.exe -d infile outfile

infile= name of the file that will be decompressed
outfile= name of the file that will be contain the decompressed data

---------------------------------------------------------------------
Example of usage to compress a file using the HUFF compression:
ea_compression_tool.exe -c -0 HUFF infile outfile

Compress using the HUFF compression the infile data and save the
compressed data to outfile. The -0 compress the data with the HUFF
variant used on NFS Most Wanted and NFS Carbon games.

---------------------------------------------------------------------
Example of usage to compress a file using the JDLZ compression:
ea_compression_tool.exe -c JDLZ infile outfile

Compress using the JDLZ compression the infile data and save the
compressed data to outfile

---------------------------------------------------------------------
More details about the other compression formats, use the -h option
ea_compression_tool.exe -h
