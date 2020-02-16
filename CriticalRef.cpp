#include <stdio.h>
// Declares clang::SyntaxOnlyAction.
#include "clang/Tooling/CommonOptionsParser.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "clang/AST/ASTImporter.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/Tooling.h"

#include <queue>
using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace ast_matchers;

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

static std::map<std::string, FunctionDecl *> FunctionDeclWithDefinitnionMaps;
static std::set<std::__cxx11::string> ResultGlobalVariableSet;
static std::set<std::__cxx11::string> ResultUntraversedFunctionSet;

class CollectFunctionDeclWithDefinitnionVisitor
  : public RecursiveASTVisitor<CollectFunctionDeclWithDefinitnionVisitor>{
public:
  explicit CollectFunctionDeclWithDefinitnionVisitor(ASTContext *Context)
    : Context(Context) {}
  bool VisitFunctionDecl(FunctionDecl *CurrentFunction) {
    auto CurrenctFunctionDef = CurrentFunction->getDefinition();
    auto CurrentName = CurrentFunction->getDeclName().getAsString();
//    llvm::outs() << "[+]Collector: collecting " << CurrentName << "\n";
    if (CurrenctFunctionDef){
      if(CurrenctFunctionDef->hasBody()){
        FunctionDeclWithDefinitnionMaps.insert(std::pair<std::string, FunctionDecl *>(CurrentName, CurrentFunction));
      }
      else{
//      llvm::outs() << "[-]Collector: " << CurrentName << " has null Body" << "\n";
      }
    }
    else{
//      llvm::outs() << "[-]Collector: " << CurrentName << " has null declaration" << "\n";
    }
    return true;
  }
private:
  ASTContext *Context;
};


class GetFunctionGlobalReferencesVisitor
  : public RecursiveASTVisitor<GetFunctionGlobalReferencesVisitor> {
public:
  explicit GetFunctionGlobalReferencesVisitor(ASTContext *Context)
    : Context(Context) {}
  
  FunctionDecl *FindFunctionDeclByName(std::string QueryName){
    if(FunctionDeclWithDefinitnionMaps.find(QueryName) == FunctionDeclWithDefinitnionMaps.end()){
//      llvm::outs() << "[-]Finder: " << QueryName << " is not existing" << "\n";
      return nullptr;
    }
    else{
      return FunctionDeclWithDefinitnionMaps[QueryName];
    }
  }
  
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
//      llvm::outs() << "Debuging" << "\n";
////      child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
//      llvm::outs() << "\n";
      //if (child->getStmtClass() == Stmt::DeclRefExprClass){
      if (child != nullptr){
        if (isa<DeclRefExpr>(child)){
          bool StoreToSet = false;
          //getReductionInit
          auto *DRE = dyn_cast<DeclRefExpr>(child);
          ValueDecl *VD = DRE->getDecl();
          bool isFunction = VD->getType()->isFunctionType();
          std::__cxx11::string CurrentName = DRE->getNameInfo().getAsString();
          if(isFunction){
            auto *FD =  dyn_cast<FunctionDecl>(VD);
//            llvm::outs() << "[+]Function" << "\n";
//            llvm::outs() << "name: " << CurrentName << "\n";
////            child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
//            llvm::outs() << "\n";
            FunctionQuene.push(FD);
            StoreToSet = true;
          }
          else{
            auto CurrentKind = VD->getKind();
//            llvm::outs() << "[+]Kind:" << "\n";
//            llvm::outs() << CurrentKind << "\n";
//            llvm::outs() << VD->getDeclKindName() << "\n";
            switch(CurrentKind){
              case 65: //EnumConstant
//                llvm::outs() << "[+]EnumConstant:" << "\n";
//                llvm::outs() << VD->getNameAsString() << "\n";
//                llvm::outs() << "name: " << CurrentName << "\n";
////                child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
//                llvm::outs() << "\n";
                StoreToSet = true;
              break;
              default:
                auto *VarD = dyn_cast<VarDecl>(VD);
//                llvm::outs() << "[+]DeclName:" << "\n";
//                llvm::outs() << VD->getDeclKindName() << "\n";
                if(VarD->hasGlobalStorage()){
//                  llvm::outs() << "[+]Global Variable" << "\n";
//                  llvm::outs() << "name: " << CurrentName << "\n";
////                  child->getBeginLoc().print(llvm::outs(), Context->getSourceManager());
//                  llvm::outs() << "\n";
                  StoreToSet = true;
                }
              break;
            }
          }

          if (StoreToSet){
            GlobalVariableSet.insert(CurrentName);
          }
        }
      }
      else{
        continue;
      }
      MyFunctionStmtVisitor(*it);
    }

    return true;
  }

  bool MyFunctionDeclTraversal(){
    while(!FunctionQuene.empty()){
      FunctionDecl *CurrentFunction = FunctionQuene.front();
      FunctionQuene.pop();
      auto CurrentName = CurrentFunction->getDeclName().getAsString();
//      llvm::outs() << "[+]CurrentName:\n";
//      llvm::outs() << CurrentName << "\n";
      auto CheckedPos = FunctionNameSet.find(CurrentName);

      if(CheckedPos == FunctionNameSet.end()){
        FunctionNameSet.insert(CurrentName);
//        llvm::outs() << "\n[+]Checking function " << CurrentName << "\n";
        //getchar();
        auto WithDefPos = FunctionDeclWithDefinitnionMaps.find(CurrentName);
        if(WithDefPos != FunctionDeclWithDefinitnionMaps.end()){
          CurrentFunction = FunctionDeclWithDefinitnionMaps[CurrentName];
          auto CurrenctFunctionDef = CurrentFunction->getDefinition();
          if(CurrenctFunctionDef->hasBody()){
            MyFunctionStmtVisitor(CurrenctFunctionDef->getBody());
          }
          else{
//            llvm::outs() << "[-]Visitor: " << CurrentName << " has null Body" << "\n";
          }
        }
        else{
          UntraversedFunctionSet.insert(CurrentName);
//          llvm::outs() << "[-]Visitor: " << CurrentName << " not found" << "\n";
        }
      }
      else{
//        llvm::outs() << "[+]" << CurrentName << " has been checked" << "\n";
      }
    }
    return true;
  }

  const std::set<std::__cxx11::string>& getGlobalVariableSet(){
    return GlobalVariableSet;
  }
  const std::set<std::__cxx11::string>& getUntraversedFunctionSet(){
    return UntraversedFunctionSet;
  }

protected:
// are there more than one instance of GetFunctionGlobalReferencesVisitor? 
  std::queue<FunctionDecl *> FunctionQuene;
  std::set<std::__cxx11::string> FunctionNameSet;
  std::set<std::__cxx11::string> GlobalVariableSet;
  std::set<std::__cxx11::string> UntraversedFunctionSet;
private:
  ASTContext *Context;
};

class FindNamedClassConsumer : public clang::ASTConsumer {
public:
  explicit FindNamedClassConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    llvm::outs() << "[+]AST Traverse Finished" << "\n";
    const std::set<std::__cxx11::string>& TempGlobalVariableSet = Visitor.getGlobalVariableSet();
    const std::set<std::__cxx11::string>& TempUntraversedFunctionSet = Visitor.getUntraversedFunctionSet();
    ResultGlobalVariableSet.insert(TempGlobalVariableSet.begin(), TempGlobalVariableSet.end());
    ResultUntraversedFunctionSet.insert(TempUntraversedFunctionSet.begin(), TempUntraversedFunctionSet.end());
  }
  
private:
  GetFunctionGlobalReferencesVisitor Visitor;
};

class FindNamedClassAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(
        new FindNamedClassConsumer(&Compiler.getASTContext()));
  }
  
};

void DumpResult()
{
  llvm::outs() << "[+]Collected Names" << "\n";
  for(const auto& Name:ResultGlobalVariableSet){
    llvm::outs() << Name << ", ";
  }
  llvm::outs() << "\n";
  llvm::outs() << "[-]Untraversed Functions" << "\n";
  for(const auto& Name:ResultUntraversedFunctionSet){
    llvm::outs() << Name << ", ";
  }
  llvm::outs() << "\n";
}

int main(int argc, const char **argv) {

  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  std::vector<std::unique_ptr<ASTUnit>> ASTs;
  int Status = Tool.buildASTs(ASTs);
  int ASTStatus = 0;
  if (Status == 1) {
    // Building ASTs failed.
    return 1;
  } else if (Status == 2) {
    ASTStatus |= 1;
    llvm::errs() << "Failed to build AST for some of the files, "
                 << "results may be incomplete."
                 << "\n";
  } else {
    assert(Status == 0 && "Unexpected status returned");
  }
  for(const auto& CurUnit: ASTs){
    CollectFunctionDeclWithDefinitnionVisitor Visitor(&CurUnit->getASTContext());
    Visitor.TraverseDecl(CurUnit->getASTContext().getTranslationUnitDecl());
  }

  Tool.run(newFrontendActionFactory<FindNamedClassAction>().get());

  DumpResult();
  return 0;
}