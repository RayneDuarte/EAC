package EAC.HUFF.example;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.foreign.ValueLayout;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Example Java program demonstrating how to use the EA Compression native
 * library
 * 
 * Usage: java LibExample.java <input file> <output file>
 */
public class LibExample {

    public static final int EA_FORMAT_HUFF = 0;
    public static final int EA_FORMAT_JDLZ = 1;
    public static final int EA_FORMAT_REF = 2;
    public static final int EA_FORMAT_BTREE = 3;
    public static final int EA_FORMAT_COMP = 4;
    public static final int EA_FORMAT_UNKNOWN = -1;

    public static String printFormat(int eaFormat) {
        return switch (eaFormat) {
            case EA_FORMAT_HUFF -> "EA_FORMAT_HUFF";
            case EA_FORMAT_JDLZ -> "EA_FORMAT_JDLZ";
            case EA_FORMAT_REF -> "EA_FORMAT_REF";
            case EA_FORMAT_BTREE -> "EA_FORMAT_BTREE";
            case EA_FORMAT_COMP -> "EA_FORMAT_COMP";
            default -> "EA_FORMAT_UNKNOWN";
        };
    }

    public static void main(String[] args) throws Throwable {
        if (args.length != 2) {
            System.err.println("Usage: java LibExample <input file> <output file>");
            System.exit(1);
        }

        String inputFile = args[0];
        String outputFile = args[1];

        if (inputFile.isEmpty() || outputFile.isEmpty()) {
            System.err.println("Input and output file paths must not be empty.");
            System.exit(1);
        }
        var pathToInput = Path.of(inputFile);
        var pathToOutput = Path.of(outputFile);

        if (!Files.exists(pathToInput) || !Files.isReadable(pathToInput)) {
            System.err.println("Input file does not exist or is not readable: " + inputFile);
            System.exit(1);
        }

        try (Arena arena = Arena.ofConfined()) {
            Linker linker = Linker.nativeLinker();
            SymbolLookup libLookup = SymbolLookup.libraryLookup("./libea_compression.so.1.0.0", arena);

            var ea_detect_format = linker.downcallHandle(
                    libLookup.find("ea_detect_format").orElseThrow(),
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.ADDRESS,
                            ValueLayout.JAVA_INT));

            var ea_get_decompressed_size = linker.downcallHandle(
                    libLookup.find("ea_get_decompressed_size").orElseThrow(),
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.ADDRESS,
                            ValueLayout.JAVA_INT));

            var ea_decompress = linker.downcallHandle(
                    libLookup.find("ea_decompress").orElseThrow(),
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.ADDRESS,
                            ValueLayout.JAVA_INT,
                            ValueLayout.ADDRESS,
                            ValueLayout.JAVA_INT));

            byte[] inputData = Files.readAllBytes(pathToInput);
            int dataSize = inputData.length;

            MemorySegment dataSegment = arena.allocateFrom(ValueLayout.JAVA_BYTE, inputData);

            int format = (int) ea_detect_format.invokeExact(
                    dataSegment,
                    dataSize);

            System.out.println("Detected format: " + printFormat(format));
            if (format == EA_FORMAT_UNKNOWN) {
                System.err.println("Unknown or unsupported EA compression format.");
                return;
            }

            int decompressedSize = (int) ea_get_decompressed_size.invokeExact(
                    dataSegment,
                    dataSize);

            System.out.println("Decompressed size: " + decompressedSize);

            MemorySegment outputSegment = arena.allocate(decompressedSize);

            int _ = (int) ea_decompress.invokeExact(
                    dataSegment,
                    dataSize,
                    outputSegment,
                    decompressedSize);

            byte[] outputData = new byte[decompressedSize];
            outputSegment.asByteBuffer().get(outputData);

            Files.write(pathToOutput, outputData);
            System.out.println("Decompression successful. Output written to: " + outputFile);
        }

    }
}