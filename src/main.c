#include "mipsasm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "1.0.0"

void print_usage(const char *prog_name) {
  printf("MIPS Assembler v%s\n", VERSION);
  printf("Usage: %s [options] input_file [output_file]\n", prog_name);
  printf("Options:\n");
  printf("  -h, --help         Show this help message\n");
  printf("  -o <file>          Specify output file\n");
  printf("  -v, --verbose      Enable verbose output\n");
}

int main(int argc, char *argv[]) {
  char *input_file = NULL;
  char *output_file = NULL;
  int verbose = 0;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "-v") == 0 ||
               strcmp(argv[i], "--verbose") == 0) {
      verbose = 1;
    } else if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 < argc) {
        output_file = argv[++i];
      } else {
        fprintf(stderr, "Error: -o option requires an argument\n");
        return 1;
      }
    } else if (input_file == NULL) {
      input_file = argv[i];
    } else if (output_file == NULL) {
      output_file = argv[i];
    }
  }

  if (input_file == NULL) {
    print_usage(argv[0]);
    return 1;
  }

  // Use default output file name if not specified
  if (output_file == NULL) {
    output_file = "output.bin";
  }

  // Assemble source file

  // Read input file
  FILE *input = fopen(input_file, "r");
  if (!input) {
    fprintf(stderr, "Error: Failed to open input file '%s'\n", input_file);
    return 1;
  }

  // Get file size
  fseek(input, 0, SEEK_END);
  long input_size = ftell(input);
  fseek(input, 0, SEEK_SET);

  if (input_size <= 0 || input_size > MAX_ASM_SIZE) {
    fprintf(stderr, "Error: Input file too large or empty\n");
    fclose(input);
    return 1;
  }

  // Read source code
  char *source_code = malloc(input_size + 1);
  if (!source_code) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    fclose(input);
    return 1;
  }

  size_t bytes_read = fread(source_code, 1, input_size, input);
  fclose(input);

  if (bytes_read != (size_t)input_size) {
    fprintf(stderr, "Error: Failed to read input file\n");
    free(source_code);
    return 1;
  }

  source_code[input_size] = '\0';

  // Assemble source code
  uint8_t *output_data;
  size_t output_size;

  if (!mips_assemble(source_code, &output_data, &output_size, verbose)) {
    fprintf(stderr, "Error: Assembly failed\n");
    free(source_code);
    return 1;
  }

  free(source_code);

  // Write output to binary file
  if (!write_binary_file(output_file, output_data, output_size)) {
    fprintf(stderr, "Error: Failed to write output file '%s'\n", output_file);
    free(output_data);
    return 1;
  }

  if (verbose) {
    printf("Assembly complete: %s -> %s\n", input_file, output_file);
    printf("Output size: %zu bytes (%zu instructions)\n", output_size,
           output_size / 4);
  }

  free(output_data);

  return 0;
}
