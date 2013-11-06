#!/usr/bin/env ruby

pattern = ARGV[0]
file = ARGV[1]

open(file) do |f|
  f.each_line do |line|
    if line.index pattern
      print line
    end
  end
end
