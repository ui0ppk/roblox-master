wsdl2h -o generated/prototypes.h RCCService.wsdl
soapcpp2 -x -i -S -Iimport -dgenerated generated/prototypes.h