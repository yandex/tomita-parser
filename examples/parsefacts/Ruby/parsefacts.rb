#!/usr/bin/env ruby

# Please make sure that the `beefcake` gem is installed and
# the `generate.sh` script has been executed.

require 'beefcake'
require_relative 'facts.pb'

def read_varint32(f)
  offset, result = 0, 0

  until f.eof? || (byte = f.readbyte).zero?
    result = if offset.zero?
      byte & 0x7F
    else
      result | ((byte & 0x7F) << offset)
    end

    offset += 7
    break if (byte & 0x80).zero?
  end

  result
end

ARGV.each do |path|
  File.open(path) do |f|
    until (size = read_varint32(f)).zero?
      data = f.read(size)
      document = TDocument.decode(data)

      puts 'id = %d' % document.Id
      puts 'url = "%s"' % document.Url

      document.FactGroup.each do |fg|
        puts 'fact type = "%s"' % fg.Type

        fg.Fact.each do |fact|
          puts 'index = %d' % fact.Index

          fact.Field.each do |field|
            puts '%s = "%s"' % [field.Name, field.Value]
          end
        end
      end
    end
  end
end
