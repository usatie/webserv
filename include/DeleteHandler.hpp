#ifndef DELETE_HANDLER_HPP
#define DELETE_HANDLER_HPP

class Connection;

class DeleteHandler {
 private:
  // Constructor/Destructor/Assignment Operator
  DeleteHandler() throw();                               // Do not implement this
  DeleteHandler(const DeleteHandler&) throw();             // Do not implement this
  DeleteHandler& operator=(const DeleteHandler&) throw();  // Do not implement this
  ~DeleteHandler() throw();                              // Do not implement this

 public:
  // Member fuctions
  static void handle(Connection& conn) throw();
};

#endif

