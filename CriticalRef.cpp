// Declares clang::SyntaxOnlyAction.
#include "clang/Tooling/CommonOptionsParser.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

#include <queue>
using namespace clang;
using namespace clang::tooling;
using namespace llvm;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");




//#define TARGET_FUNCTION_NAME "addFive"
#define TARGET_FUNCTION_NAME "process_vm_rw"

class FindNamedClassVisitor
  : public RecursiveASTVisitor<FindNamedClassVisitor> {
public:
  explicit FindNamedClassVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitFunctionDecl(FunctionDecl *CurrentFunction) {
    if (CurrentFunction->getDeclName().getAsString() == TARGET_FUNCTION_NAME){
      FunctionQuene.push(CurrentFunction);
      MyFunctionDeclTraversal();
      return false;
    }
    else{
      return true;
    }
  }

  bool MyFunctionStmtVisitor(Stmt *CurrentStmt)
  {
    auto Body = CurrentStmt;
    for (Stmt::child_iterator it = Body->child_begin(); it != Body->child_end(); it++){
      Stmt *child = *it;
      llvm::outs() << "Debuging" << "\n";
      child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
      llvm::outs() << "\n";
      //if (child->getStmtClass() == Stmt::DeclRefExprClass){
      if (isa<DeclRefExpr>(child)){
        //getReductionInit
        auto *DRE = dyn_cast<DeclRefExpr>(child);
        ValueDecl *VD = DRE->getDecl();
        bool isFunction = VD->getType()->isFunctionType();
        if(isFunction){
          auto *FD =  dyn_cast<FunctionDecl>(VD);
          llvm::outs() << "[+]Function" << "\n";
          llvm::outs() << "name: " << DRE->getNameInfo().getAsString() << "\n";
          child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
          llvm::outs() << "\n";
          FunctionQuene.push(FD);
        }
        else{
          auto CurrentKind = VD->getKind();
          llvm::outs() << "[+]Kind:" << "\n";
          llvm::outs() << CurrentKind << "\n";
          llvm::outs() << VD->getDeclKindName() << "\n";
          switch(CurrentKind){
            case 65: //EnumConstant
              llvm::outs() << "[+]EnumConstant:" << "\n";
              llvm::outs() << VD->getNameAsString() << "\n";
              llvm::outs() << "name: " << DRE->getNameInfo().getAsString() << "\n";
              child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
              llvm::outs() << "\n";
            break;
            default:
              auto *VarD = dyn_cast<VarDecl>(VD);
              llvm::outs() << "[+]DeclName:" << "\n";
              llvm::outs() << VD->getDeclKindName() << "\n";
              if(VarD->hasGlobalStorage()){
                llvm::outs() << "[+]Global Variable" << "\n";
                llvm::outs() << "name: " << DRE->getNameInfo().getAsString() << "\n";
                child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
                llvm::outs() << "\n";
              }
            break;
          }
        }
      }
      MyFunctionStmtVisitor(*it);
    }

    return true;
  }
  //bool MyFunctionStmtVisitor(Stmt *CurrentStmt)
  //{
  //  auto Body = CurrentStmt;
  //  for (Stmt::child_iterator it = Body->child_begin(); it != Body->child_end(); it++){
  //    Stmt *child = *it;
  //    //if (child->getStmtClass() == Stmt::DeclRefExprClass){
  //    if (isa<DeclRefExpr>(child)){
  //      //getReductionInit
  //      auto *DRE = dyn_cast<DeclRefExpr>(child);
  //      ValueDecl *VD = DRE->getDecl();
  //      auto *VarD = dyn_cast<VarDecl>(VD);
  //      bool isGlobal = VarD->hasGlobalStorage();
  //      if(isGlobal){
  //        llvm::outs() << "[+]name: " << DRE->getNameInfo().getAsString() << "\n";
  //        child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
  //        llvm::outs() << "\n";
  //        bool isFunction = VD->getType()->isFunctionType();
  //        if(isFunction){
  //          llvm::outs() << "Function" << "\n\n";
  //          auto *FD =  dyn_cast<FunctionDecl>(VD);
  //          FunctionQuene.push(FD);
  //        }
  //        else{
  //          llvm::outs() << "Global Variable" << "\n\n";
  //        }
//
  //      }
  //    }
  //    MyFunctionStmtVisitor(*it);
  //  }
//
  //  return true;
  //}

  bool MyFunctionDeclTraversal(){
    while(!FunctionQuene.empty()){
      FunctionDecl *CurrentFunction = FunctionQuene.front();
      FunctionQuene.pop();
      auto CurrentName = CurrentFunction->getDeclName().getAsString();
      auto FoundPos = FunctionNameSet.find(CurrentName);
      if(FoundPos == FunctionNameSet.end()){
        FunctionNameSet.insert(CurrentName);
        llvm::outs() << "\n[+]Checking function " << CurrentName << "\n";
        auto CurrenctFunctionDecl = CurrentFunction->getDefinition();
        if (CurrenctFunctionDecl){
          if(CurrenctFunctionDecl->hasBody()){
            MyFunctionStmtVisitor(CurrenctFunctionDecl->getBody());
          }
          else{

          llvm::outs() << "[-]" << CurrentName << " has null Body" << "\n";
          }
        }
        else{
          llvm::outs() << "[-]" << CurrentName << " has null declaration" << "\n";
        }
      }
      else{
        llvm::outs() << "[+]" << CurrentName << " has been checked" << "\n";
      }
    }
    return true;
  }

protected:
// are there more than one instance of FindNamedClassVisitor? 
  std::queue<FunctionDecl *> FunctionQuene;
  std::set<std::__cxx11::string> FunctionNameSet;

private:
  ASTContext *Context;
};

class FindNamedClassConsumer : public clang::ASTConsumer {
public:
  explicit FindNamedClassConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
  
private:
  FindNamedClassVisitor Visitor;
};

class FindNamedClassAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(
        new FindNamedClassConsumer(&Compiler.getASTContext()));
  }
  
};

int main(int argc, const char **argv) {

  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  return Tool.run(newFrontendActionFactory<FindNamedClassAction>().get());
}