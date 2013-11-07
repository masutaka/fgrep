#!/usr/bin/env ruby

def print_with_filename(filename, line)
  print "#{filename}: #{line}"
end

def print_without_filename(filename, line)
  print line
end

pattern = ARGV[0]
ARGV.shift
filenames = ARGV

print = filenames.count >= 2 ? :print_with_filename : :print_without_filename

filenames.each do |filename|
  open(filename) do |f|
    f.each_line do |line|
      if line.index pattern
        method(print).call filename, line
      end
    end
  end
end
