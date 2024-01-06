#ifndef TemplatePrinter_h
  #define TemplatePrinter_h

  #include "PsychicCore.h"
  #include <Print.h>
  
  class TemplatePrinter;

  typedef std::function<bool(Print &output, const char *parameter)> TemplateCallback;
  typedef std::function<void(TemplatePrinter &printer)> TemplateSourceCallback;

  class TemplatePrinter : public Print{
    private:
      bool _inParam;
      char _paramBuffer[64];
      uint8_t _paramPos;
      Print &_stream;
      TemplateCallback _cb;
      char _delimiter;
    
      void resetParam(bool flush);
      
    public:
      using Print::write;

      static void start(Print &stream, TemplateCallback cb, TemplateSourceCallback entry){
        TemplatePrinter printer(stream, cb);
        entry(printer);
      }

      TemplatePrinter(Print &stream, TemplateCallback cb, const char delimeter = '%') : _stream(stream), _cb(cb), _delimiter(delimeter) { resetParam(false); }
      ~TemplatePrinter(){ flush(); }

      void flush() override;
      size_t write(uint8_t data) override;
      size_t copyFrom(Stream &stream);
  };

#endif
