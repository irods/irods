#!/usr/bin/env ruby
=begin

  Originally written by Adil Hasan, University of Liverpool, April 2009
  - Script to read config file and run doxygen on the iRODS source code
  - Used /config/doxygen.config
    - Generates full doxygen output (graphs, images, links, full source)

  Modified by Terrell Russell, SILS@UNC-CH, Sept 2009
  - Refactored quite a bit
  - Added second Doxyfile for cleaner, quicker, msi-only documentation
    - Only *.c files, no images
  - Set this new Doxyfile as default
  - Runs in under one minute
  - Compiles HTML
  - Can compile RTF
  - Can compile PDF from LATEX source
  - Better output/feedback

  -----

  This script checks the current $PATH for installed versions of:
    doxygen, latex, and pdflatex.

  In addition to doxygen, to run this script you may need to install:
    imagemagick, texlive, and/or texlive-latex-extra

  Run this script:
    $/iRODS/> ruby runDoxygen.rb

  You can safely re-run this script multiple times.

=end

# Check that an executable exists on the machine
def executable_exists(program_name)
  exists = false
  path_array = ENV['PATH'].split(':')
  for indx in 0...path_array.length do
    path = File.join(path_array[indx], program_name)
    if (File.exist?(path))
      exists = true
    end
  end
  return exists
end

# Read in the config file
def read_config(cfg_file, doxy_keys)
  doxy_cfg = Hash.new()
  if (!File.exist?(cfg_file)) then
    puts("Error: Cannot open Doxyfile (doxygen config file)!")
    exit(1)
  end
  file = File.new(cfg_file, "r")
  while (line=file.gets())
    if ((line =~ /#/) == nil) then
      tkey, tvalue = line.split("=")
      key = tkey.strip()
      if (tvalue): value = tvalue.strip() end
      if (doxy_keys.include?(key)): doxy_cfg[key] = value end
    end
  end
  file.close()
  return doxy_cfg
end



def main()

  # Configuration
  input_file = "./config/doxygen.config"     # original full config - Adil Hasan
  input_file = "./config/doxygen-msi.config" # just microservices (msi) - Terrell Russell

  output_file = "./config/doxygen-saved.cfg"

  doxy_keys = ["PROJECT_NAME", "PROJECT_NUMBER", "OUTPUT_DIRECTORY",
  "STRIP_FROM_PATH", "INPUT", "GENERATE_HTML", "GENERATE_RTF", "GENERATE_LATEX"]

  # Check for doxygen in path
  if (!executable_exists("doxygen"))
    puts("Error: doxygen executable not found in your path or not installed\n\n")
    exit(1)
  end
  
  # Check whether this script has been run already
  response = ""
  if (File.exist?(output_file))
    puts("\nAttention: [#{output_file}] already exists.")
    puts("\Use previous settings? y/n [y]:")
    response = gets()
  end

  # Determine whether to read config file
  if (!File.exist?(output_file) || response.strip() =~ /n/)

    # Read included config file
    doxy_cfg = read_config(input_file, doxy_keys)

    # Prompt for custom values of configuration parameters
    puts("\n---\n\nThis script can generate RTF, LATEX, and HTML documentation for iRODS.\n\n")
    puts("Enter your configuration parameter values.\nThe default values are in brackets.")
    puts("Your saved configuration values will be written to [#{output_file}]\n\n")
    for key in doxy_cfg.keys.sort.reverse
      # Skip the strip_from_path it is the same as the input
      if ((key =~ /STRIP_FROM_PATH/) != nil): next end
      if (doxy_cfg[key] != nil) then
        puts("Enter #{key} [#{doxy_cfg[key]}]:")
        tvalue = gets()
        value = tvalue.strip()
        if (value.length() > 0): doxy_cfg[key] = value end
      else
        puts("Enter #{key} []:")
        tvalue = gets()
        value = tvalue.strip()
        if (value.length() > 0): doxy_cfg[key] = value end
      end
    end

    doxy_cfg["STRIP_FROM_PATH"] = doxy_cfg["OUTPUT_DIRECTORY"]

    # Write out the customized Doxygen output file.
    in_file = File.new(input_file, "r")
    out_file = File.new(output_file, "w")
    while (line = in_file.gets()) do
      if (line =~ /=/) then
        tkey, tvalue = line.split("=")
        key = tkey.strip()
        if (doxy_keys.include?(key)) then
          line = [tkey, doxy_cfg[key]].join(" = ")
        end
      end
      out_file.puts(line)
    end
    in_file.close()
    out_file.close()

  # Entered 'y', so continue with existing saved config file
  else
    doxy_cfg = read_config(output_file, doxy_keys)
    doxy_cfg["STRIP_FROM_PATH"] = doxy_cfg["OUTPUT_DIRECTORY"]
    puts("Re-running Doxygen with the previously saved configuration...")
  end

  # Remove any earlier generated documentation
  system("rm -r #{doxy_cfg["OUTPUT_DIRECTORY"]}/html/")
  system("rm -r #{doxy_cfg["OUTPUT_DIRECTORY"]}/rtf/")
  system("rm -r #{doxy_cfg["OUTPUT_DIRECTORY"]}/latex/")

  # Run the doxygen command
  puts("Running the doxygen command to generate documentation.")
  system("doxygen #{output_file}")
  if ($? != 0) then
    puts("Error: Failed generating the documentation. Return Code: #{$?}")
  else
    puts("Finished generating documentation!")
  end

  # Run the pdflatex command, if necessary
  if (doxy_cfg["GENERATE_LATEX"] == "YES" && (executable_exists("latex") || executable_exists("pdflatex")))
    puts("Generating a pdf from the latex documentation.")
    system("cd #{doxy_cfg["OUTPUT_DIRECTORY"]}/latex; make")
    if ($? != 0) then
      puts("Error: Failed generating the pdf. Return Code: #{$?}")
    else
      puts("Finished generating pdf!")
    end
  end

  # Friendly output
  puts("\n---\n")
  if (doxy_cfg["GENERATE_RTF"] == "YES")
    puts("RTF documentation   -> #{doxy_cfg["OUTPUT_DIRECTORY"]}/rtf/refman.rtf")
  end
  if (doxy_cfg["GENERATE_LATEX"] == "YES")
    puts("LATEX documentation -> #{doxy_cfg["OUTPUT_DIRECTORY"]}/latex/")
    puts("PDF documentation   -> #{doxy_cfg["OUTPUT_DIRECTORY"]}/latex/refman.pdf")
  end
  if (doxy_cfg["GENERATE_HTML"] == "YES")
    puts("HTML documentation  -> #{doxy_cfg["OUTPUT_DIRECTORY"]}/html/index.html")
  end

end

main()
