// Declares clang::SyntaxOnlyAction.
#include "clang/Tooling/CommonOptionsParser.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

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



class FindNamedClassVisitor
  : public RecursiveASTVisitor<FindNamedClassVisitor> {
public:
  explicit FindNamedClassVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitFunctionDecl(FunctionDecl *CurrentFunction) {
    if (CurrentFunction->getDeclName().getAsString() == "addFive"){
      auto Body = CurrentFunction->getDefinition()->getBody();
      MyFunctionStmtVisitor(Body);
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
      //if (child->getStmtClass() == Stmt::DeclRefExprClass){
      if (isa<DeclRefExpr>(child)){
        //getReductionInit
        auto *DRE = dyn_cast<DeclRefExpr>(child);
        ValueDecl *VD = DRE->getDecl();
        bool isFunction = VD->getType()->isFunctionType();
        if(isFunction){
          llvm::outs() << "class name: " << child->getStmtClassName() << "\n";
          llvm::outs() << "name: " << DRE->getNameInfo().getAsString() << "\n";
          child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
          llvm::outs() << "\n";
        }
        else{
          auto *VarD = dyn_cast<VarDecl>(VD);
          if(VarD->hasGlobalStorage()){
          llvm::outs() << "class name: " << child->getStmtClassName() << "\n";
          llvm::outs() << "name: " << DRE->getNameInfo().getAsString() << "\n";
          child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
          llvm::outs() << "\n";
          }
        }
      }
      MyFunctionStmtVisitor(*it);
    }

    return true;
  }

private:
  bool Found = false;

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