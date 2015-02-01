
from facts_pb2 import TDocument
import sys

def ReadVarint32(f):
  offset = 0
  result = 0;
  byte = f.read(1)
  while byte != "":
    if 0 == offset:
      result = ord(byte) & 0x7F
    else:
      result = result | ( ( ord(byte) & 0x7F ) << offset )

    offset += 7

    if (ord(byte) & 0x80) == 0 :
      break

    byte = f.read(1)

  return result


for a in sys.argv[1:]:
  ff = open(a, "rb")
  

  size = ReadVarint32(ff)
  while (size > 0):
    data = ff.read(size)
    doc = TDocument()
    doc.ParseFromString(data)

    print "id =", doc.Id
    print "url =", doc.Url
  
    for fg in doc.FactGroup:
      print "fact type =", fg.Type

      for f in fg.Fact:
        print "index =", f.Index

        for field in f.Field:
          print field.Name, "=", field.Value

    size = ReadVarint32(ff)


  ff.close;

