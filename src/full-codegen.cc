// Copyright 2009 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "v8.h"

#include "codegen-inl.h"
#include "compiler.h"
#include "full-codegen.h"
#include "stub-cache.h"
#include "debug.h"

namespace v8 {
namespace internal {

#define BAILOUT(reason)                         \
  do {                                          \
    if (FLAG_trace_bailout) {                   \
      PrintF("%s\n", reason);                   \
    }                                           \
    has_supported_syntax_ = false;              \
    return;                                     \
  } while (false)


#define CHECK_BAILOUT                           \
  do {                                          \
    if (!has_supported_syntax_) return;         \
  } while (false)


void FullCodeGenSyntaxChecker::Check(FunctionLiteral* fun) {
  Scope* scope = fun->scope();

  if (scope->num_heap_slots() > 0) {
    // We support functions with a local context if they do not have
    // parameters that need to be copied into the context.
    for (int i = 0, len = scope->num_parameters(); i < len; i++) {
      Slot* slot = scope->parameter(i)->slot();
      if (slot != NULL && slot->type() == Slot::CONTEXT) {
        BAILOUT("Function has context-allocated parameters.");
      }
    }
  }

  VisitDeclarations(scope->declarations());
  CHECK_BAILOUT;

  VisitStatements(fun->body());
}


void FullCodeGenSyntaxChecker::VisitDeclarations(
    ZoneList<Declaration*>* decls) {
  for (int i = 0; i < decls->length(); i++) {
    Visit(decls->at(i));
    CHECK_BAILOUT;
  }
}


void FullCodeGenSyntaxChecker::VisitStatements(ZoneList<Statement*>* stmts) {
  for (int i = 0, len = stmts->length(); i < len; i++) {
    Visit(stmts->at(i));
    CHECK_BAILOUT;
  }
}


void FullCodeGenSyntaxChecker::VisitDeclaration(Declaration* decl) {
  Property* prop = decl->proxy()->AsProperty();
  if (prop != NULL) {
    Visit(prop->obj());
    Visit(prop->key());
  }

  if (decl->fun() != NULL) {
    Visit(decl->fun());
  }
}


void FullCodeGenSyntaxChecker::VisitBlock(Block* stmt) {
  VisitStatements(stmt->statements());
}


void FullCodeGenSyntaxChecker::VisitExpressionStatement(
    ExpressionStatement* stmt) {
  Visit(stmt->expression());
}


void FullCodeGenSyntaxChecker::VisitEmptyStatement(EmptyStatement* stmt) {
  // Supported.
}


void FullCodeGenSyntaxChecker::VisitIfStatement(IfStatement* stmt) {
  Visit(stmt->condition());
  CHECK_BAILOUT;
  Visit(stmt->then_statement());
  CHECK_BAILOUT;
  Visit(stmt->else_statement());
}


void FullCodeGenSyntaxChecker::VisitContinueStatement(ContinueStatement* stmt) {
  // Supported.
}


void FullCodeGenSyntaxChecker::VisitBreakStatement(BreakStatement* stmt) {}


void FullCodeGenSyntaxChecker::VisitReturnStatement(ReturnStatement* stmt) {
  Visit(stmt->expression());
}


void FullCodeGenSyntaxChecker::VisitWithEnterStatement(
    WithEnterStatement* stmt) {
  Visit(stmt->expression());
}


void FullCodeGenSyntaxChecker::VisitWithExitStatement(WithExitStatement* stmt) {
  // Supported.
}


void FullCodeGenSyntaxChecker::VisitSwitchStatement(SwitchStatement* stmt) {
  BAILOUT("SwitchStatement");
}


void FullCodeGenSyntaxChecker::VisitDoWhileStatement(DoWhileStatement* stmt) {
  Visit(stmt->cond());
  CHECK_BAILOUT;
  Visit(stmt->body());
}


void FullCodeGenSyntaxChecker::VisitWhileStatement(WhileStatement* stmt) {
  Visit(stmt->cond());
  CHECK_BAILOUT;
  Visit(stmt->body());
}


void FullCodeGenSyntaxChecker::VisitForStatement(ForStatement* stmt) {
  if (!FLAG_always_fast_compiler) BAILOUT("ForStatement");
  if (stmt->init() != NULL) {
    Visit(stmt->init());
    CHECK_BAILOUT;
  }
  if (stmt->cond() != NULL) {
    Visit(stmt->cond());
    CHECK_BAILOUT;
  }
  Visit(stmt->body());
  if (stmt->next() != NULL) {
    CHECK_BAILOUT;
    Visit(stmt->next());
  }
}


void FullCodeGenSyntaxChecker::VisitForInStatement(ForInStatement* stmt) {
  BAILOUT("ForInStatement");
}


void FullCodeGenSyntaxChecker::VisitTryCatchStatement(TryCatchStatement* stmt) {
  Visit(stmt->try_block());
  CHECK_BAILOUT;
  Visit(stmt->catch_block());
}


void FullCodeGenSyntaxChecker::VisitTryFinallyStatement(
    TryFinallyStatement* stmt) {
  Visit(stmt->try_block());
  CHECK_BAILOUT;
  Visit(stmt->finally_block());
}


void FullCodeGenSyntaxChecker::VisitDebuggerStatement(
    DebuggerStatement* stmt) {
  // Supported.
}


void FullCodeGenSyntaxChecker::VisitFunctionLiteral(FunctionLiteral* expr) {
  // Supported.
}


void FullCodeGenSyntaxChecker::VisitFunctionBoilerplateLiteral(
    FunctionBoilerplateLiteral* expr) {
  BAILOUT("FunctionBoilerplateLiteral");
}


void FullCodeGenSyntaxChecker::VisitConditional(Conditional* expr) {
  Visit(expr->condition());
  CHECK_BAILOUT;
  Visit(expr->then_expression());
  CHECK_BAILOUT;
  Visit(expr->else_expression());
}


void FullCodeGenSyntaxChecker::VisitSlot(Slot* expr) {
  UNREACHABLE();
}


void FullCodeGenSyntaxChecker::VisitVariableProxy(VariableProxy* expr) {
  Variable* var = expr->var();
  if (!var->is_global()) {
    Slot* slot = var->slot();
    if (slot != NULL) {
      Slot::Type type = slot->type();
      // When LOOKUP slots are enabled, some currently dead code
      // implementing unary typeof will become live.
      if (type == Slot::LOOKUP) {
        BAILOUT("Lookup slot");
      }
    } else {
      // If not global or a slot, it is a parameter rewritten to an explicit
      // property reference on the (shadow) arguments object.
#ifdef DEBUG
      Property* property = var->AsProperty();
      ASSERT_NOT_NULL(property);
      Variable* object = property->obj()->AsVariableProxy()->AsVariable();
      ASSERT_NOT_NULL(object);
      ASSERT_NOT_NULL(object->slot());
      ASSERT_NOT_NULL(property->key()->AsLiteral());
      ASSERT(property->key()->AsLiteral()->handle()->IsSmi());
#endif
    }
  }
}


void FullCodeGenSyntaxChecker::VisitLiteral(Literal* expr) {
  // Supported.
}


void FullCodeGenSyntaxChecker::VisitRegExpLiteral(RegExpLiteral* expr) {
  // Supported.
}


void FullCodeGenSyntaxChecker::VisitObjectLiteral(ObjectLiteral* expr) {
  ZoneList<ObjectLiteral::Property*>* properties = expr->properties();

  for (int i = 0, len = properties->length(); i < len; i++) {
    ObjectLiteral::Property* property = properties->at(i);
    if (property->IsCompileTimeValue()) continue;
    Visit(property->key());
    CHECK_BAILOUT;
    Visit(property->value());
    CHECK_BAILOUT;
  }
}


void FullCodeGenSyntaxChecker::VisitArrayLiteral(ArrayLiteral* expr) {
  ZoneList<Expression*>* subexprs = expr->values();
  for (int i = 0, len = subexprs->length(); i < len; i++) {
    Expression* subexpr = subexprs->at(i);
    if (subexpr->AsLiteral() != NULL) continue;
    if (CompileTimeValue::IsCompileTimeValue(subexpr)) continue;
    Visit(subexpr);
    CHECK_BAILOUT;
  }
}


void FullCodeGenSyntaxChecker::VisitCatchExtensionObject(
    CatchExtensionObject* expr) {
  Visit(expr->key());
  CHECK_BAILOUT;
  Visit(expr->value());
}


void FullCodeGenSyntaxChecker::VisitAssignment(Assignment* expr) {
  // We support plain non-compound assignments to properties, parameters and
  // non-context (stack-allocated) locals, and global variables.
  Token::Value op = expr->op();
  if (op == Token::INIT_CONST) BAILOUT("initialize constant");

  Variable* var = expr->target()->AsVariableProxy()->AsVariable();
  Property* prop = expr->target()->AsProperty();
  ASSERT(var == NULL || prop == NULL);
  if (var != NULL) {
    if (var->mode() == Variable::CONST) {
      BAILOUT("Assignment to const");
    }
    // All global variables are supported.
    if (!var->is_global()) {
      ASSERT(var->slot() != NULL);
      Slot::Type type = var->slot()->type();
      if (type == Slot::LOOKUP) {
        BAILOUT("Lookup slot");
      }
    }
  } else if (prop != NULL) {
    Visit(prop->obj());
    CHECK_BAILOUT;
    Visit(prop->key());
    CHECK_BAILOUT;
  } else {
    // This is a throw reference error.
    BAILOUT("non-variable/non-property assignment");
  }

  Visit(expr->value());
}


void FullCodeGenSyntaxChecker::VisitThrow(Throw* expr) {
  Visit(expr->exception());
}


void FullCodeGenSyntaxChecker::VisitProperty(Property* expr) {
  Visit(expr->obj());
  CHECK_BAILOUT;
  Visit(expr->key());
}


void FullCodeGenSyntaxChecker::VisitCall(Call* expr) {
  Expression* fun = expr->expression();
  ZoneList<Expression*>* args = expr->arguments();
  Variable* var = fun->AsVariableProxy()->AsVariable();

  // Check for supported calls
  if (var != NULL && var->is_possibly_eval()) {
    BAILOUT("call to the identifier 'eval'");
  } else if (var != NULL && !var->is_this() && var->is_global()) {
    // Calls to global variables are supported.
  } else if (var != NULL && var->slot() != NULL &&
             var->slot()->type() == Slot::LOOKUP) {
    BAILOUT("call to a lookup slot");
  } else if (fun->AsProperty() != NULL) {
    Property* prop = fun->AsProperty();
    Visit(prop->obj());
    CHECK_BAILOUT;
    Visit(prop->key());
    CHECK_BAILOUT;
  } else {
    // Otherwise the call is supported if the function expression is.
    Visit(fun);
  }
  // Check all arguments to the call.
  for (int i = 0; i < args->length(); i++) {
    Visit(args->at(i));
    CHECK_BAILOUT;
  }
}


void FullCodeGenSyntaxChecker::VisitCallNew(CallNew* expr) {
  Visit(expr->expression());
  CHECK_BAILOUT;
  ZoneList<Expression*>* args = expr->arguments();
  // Check all arguments to the call
  for (int i = 0; i < args->length(); i++) {
    Visit(args->at(i));
    CHECK_BAILOUT;
  }
}


void FullCodeGenSyntaxChecker::VisitCallRuntime(CallRuntime* expr) {
  // Check for inline runtime call
  if (expr->name()->Get(0) == '_' &&
      CodeGenerator::FindInlineRuntimeLUT(expr->name()) != NULL) {
    BAILOUT("inlined runtime call");
  }
  // Check all arguments to the call.  (Relies on TEMP meaning STACK.)
  for (int i = 0; i < expr->arguments()->length(); i++) {
    Visit(expr->arguments()->at(i));
    CHECK_BAILOUT;
  }
}


void FullCodeGenSyntaxChecker::VisitUnaryOperation(UnaryOperation* expr) {
  switch (expr->op()) {
    case Token::VOID:
    case Token::NOT:
    case Token::TYPEOF:
      Visit(expr->expression());
      break;
    case Token::BIT_NOT:
      BAILOUT("UnaryOperation: BIT_NOT");
    case Token::DELETE:
      BAILOUT("UnaryOperation: DELETE");
    case Token::ADD:
      BAILOUT("UnaryOperation: ADD");
    case Token::SUB:
      BAILOUT("UnaryOperation: SUB");
    default:
      UNREACHABLE();
  }
}


void FullCodeGenSyntaxChecker::VisitCountOperation(CountOperation* expr) {
  Variable* var = expr->expression()->AsVariableProxy()->AsVariable();
  Property* prop = expr->expression()->AsProperty();
  ASSERT(var == NULL || prop == NULL);
  if (var != NULL) {
    // All global variables are supported.
    if (!var->is_global()) {
      ASSERT(var->slot() != NULL);
      Slot::Type type = var->slot()->type();
      if (type == Slot::LOOKUP) {
        BAILOUT("CountOperation with lookup slot");
      }
    }
  } else if (prop != NULL) {
    Visit(prop->obj());
    CHECK_BAILOUT;
    Visit(prop->key());
    CHECK_BAILOUT;
  } else {
    // This is a throw reference error.
    BAILOUT("CountOperation non-variable/non-property expression");
  }
}


void FullCodeGenSyntaxChecker::VisitBinaryOperation(BinaryOperation* expr) {
  Visit(expr->left());
  CHECK_BAILOUT;
  Visit(expr->right());
}


void FullCodeGenSyntaxChecker::VisitCompareOperation(CompareOperation* expr) {
  Visit(expr->left());
  CHECK_BAILOUT;
  Visit(expr->right());
}


void FullCodeGenSyntaxChecker::VisitThisFunction(ThisFunction* expr) {
  // Supported.
}

#undef BAILOUT
#undef CHECK_BAILOUT


#define __ ACCESS_MASM(masm())

Handle<Code> FullCodeGenerator::MakeCode(FunctionLiteral* fun,
                                         Handle<Script> script,
                                         bool is_eval) {
  CodeGenerator::MakeCodePrologue(fun);
  const int kInitialBufferSize = 4 * KB;
  MacroAssembler masm(NULL, kInitialBufferSize);
  FullCodeGenerator cgen(&masm, script, is_eval);
  cgen.Generate(fun);
  if (cgen.HasStackOverflow()) {
    ASSERT(!Top::has_pending_exception());
    return Handle<Code>::null();
  }
  Code::Flags flags = Code::ComputeFlags(Code::FUNCTION, NOT_IN_LOOP);
  return CodeGenerator::MakeCodeEpilogue(fun, &masm, flags, script);
}


int FullCodeGenerator::SlotOffset(Slot* slot) {
  ASSERT(slot != NULL);
  // Offset is negative because higher indexes are at lower addresses.
  int offset = -slot->index() * kPointerSize;
  // Adjust by a (parameter or local) base offset.
  switch (slot->type()) {
    case Slot::PARAMETER:
      offset += (function_->scope()->num_parameters() + 1) * kPointerSize;
      break;
    case Slot::LOCAL:
      offset += JavaScriptFrameConstants::kLocal0Offset;
      break;
    case Slot::CONTEXT:
    case Slot::LOOKUP:
      UNREACHABLE();
  }
  return offset;
}


void FullCodeGenerator::VisitDeclarations(
    ZoneList<Declaration*>* declarations) {
  int length = declarations->length();
  int globals = 0;
  for (int i = 0; i < length; i++) {
    Declaration* decl = declarations->at(i);
    Variable* var = decl->proxy()->var();
    Slot* slot = var->slot();

    // If it was not possible to allocate the variable at compile
    // time, we need to "declare" it at runtime to make sure it
    // actually exists in the local context.
    if ((slot != NULL && slot->type() == Slot::LOOKUP) || !var->is_global()) {
      VisitDeclaration(decl);
    } else {
      // Count global variables and functions for later processing
      globals++;
    }
  }

  // Compute array of global variable and function declarations.
  // Do nothing in case of no declared global functions or variables.
  if (globals > 0) {
    Handle<FixedArray> array = Factory::NewFixedArray(2 * globals, TENURED);
    for (int j = 0, i = 0; i < length; i++) {
      Declaration* decl = declarations->at(i);
      Variable* var = decl->proxy()->var();
      Slot* slot = var->slot();

      if ((slot == NULL || slot->type() != Slot::LOOKUP) && var->is_global()) {
        array->set(j++, *(var->name()));
        if (decl->fun() == NULL) {
          if (var->mode() == Variable::CONST) {
            // In case this is const property use the hole.
            array->set_the_hole(j++);
          } else {
            array->set_undefined(j++);
          }
        } else {
          Handle<JSFunction> function =
              Compiler::BuildBoilerplate(decl->fun(), script_, this);
          // Check for stack-overflow exception.
          if (HasStackOverflow()) return;
          array->set(j++, *function);
        }
      }
    }
    // Invoke the platform-dependent code generator to do the actual
    // declaration the global variables and functions.
    DeclareGlobals(array);
  }
}


void FullCodeGenerator::SetFunctionPosition(FunctionLiteral* fun) {
  if (FLAG_debug_info) {
    CodeGenerator::RecordPositions(masm_, fun->start_position());
  }
}


void FullCodeGenerator::SetReturnPosition(FunctionLiteral* fun) {
  if (FLAG_debug_info) {
    CodeGenerator::RecordPositions(masm_, fun->end_position());
  }
}


void FullCodeGenerator::SetStatementPosition(Statement* stmt) {
  if (FLAG_debug_info) {
    CodeGenerator::RecordPositions(masm_, stmt->statement_pos());
  }
}


void FullCodeGenerator::SetStatementPosition(int pos) {
  if (FLAG_debug_info) {
    CodeGenerator::RecordPositions(masm_, pos);
  }
}


void FullCodeGenerator::SetSourcePosition(int pos) {
  if (FLAG_debug_info && pos != RelocInfo::kNoPosition) {
    masm_->RecordPosition(pos);
  }
}


void FullCodeGenerator::EmitLogicalOperation(BinaryOperation* expr) {
  Label eval_right, done;

  // Set up the appropriate context for the left subexpression based
  // on the operation and our own context.  Initially assume we can
  // inherit both true and false labels from our context.
  if (expr->op() == Token::OR) {
    switch (context_) {
      case Expression::kUninitialized:
        UNREACHABLE();
      case Expression::kEffect:
        VisitForControl(expr->left(), &done, &eval_right);
        break;
      case Expression::kValue:
        VisitForValueControl(expr->left(),
                             location_,
                             &done,
                             &eval_right);
        break;
      case Expression::kTest:
        VisitForControl(expr->left(), true_label_, &eval_right);
        break;
      case Expression::kValueTest:
        VisitForValueControl(expr->left(),
                             location_,
                             true_label_,
                             &eval_right);
        break;
      case Expression::kTestValue:
        VisitForControl(expr->left(), true_label_, &eval_right);
        break;
    }
  } else {
    ASSERT_EQ(Token::AND, expr->op());
    switch (context_) {
      case Expression::kUninitialized:
        UNREACHABLE();
      case Expression::kEffect:
        VisitForControl(expr->left(), &eval_right, &done);
        break;
      case Expression::kValue:
        VisitForControlValue(expr->left(),
                             location_,
                             &eval_right,
                             &done);
        break;
      case Expression::kTest:
        VisitForControl(expr->left(), &eval_right, false_label_);
        break;
      case Expression::kValueTest:
        VisitForControl(expr->left(), &eval_right, false_label_);
        break;
      case Expression::kTestValue:
        VisitForControlValue(expr->left(),
                             location_,
                             &eval_right,
                             false_label_);
        break;
    }
  }

  __ bind(&eval_right);
  Visit(expr->right());

  __ bind(&done);
}


void FullCodeGenerator::VisitBlock(Block* stmt) {
  Comment cmnt(masm_, "[ Block");
  Breakable nested_statement(this, stmt);
  SetStatementPosition(stmt);
  VisitStatements(stmt->statements());
  __ bind(nested_statement.break_target());
}


void FullCodeGenerator::VisitExpressionStatement(ExpressionStatement* stmt) {
  Comment cmnt(masm_, "[ ExpressionStatement");
  SetStatementPosition(stmt);
  VisitForEffect(stmt->expression());
}


void FullCodeGenerator::VisitEmptyStatement(EmptyStatement* stmt) {
  Comment cmnt(masm_, "[ EmptyStatement");
  SetStatementPosition(stmt);
}


void FullCodeGenerator::VisitIfStatement(IfStatement* stmt) {
  Comment cmnt(masm_, "[ IfStatement");
  SetStatementPosition(stmt);
  Label then_part, else_part, done;

  // Do not worry about optimizing for empty then or else bodies.
  VisitForControl(stmt->condition(), &then_part, &else_part);

  __ bind(&then_part);
  Visit(stmt->then_statement());
  __ jmp(&done);

  __ bind(&else_part);
  Visit(stmt->else_statement());

  __ bind(&done);
}


void FullCodeGenerator::VisitContinueStatement(ContinueStatement* stmt) {
  Comment cmnt(masm_,  "[ ContinueStatement");
  SetStatementPosition(stmt);
  NestedStatement* current = nesting_stack_;
  int stack_depth = 0;
  while (!current->IsContinueTarget(stmt->target())) {
    stack_depth = current->Exit(stack_depth);
    current = current->outer();
  }
  __ Drop(stack_depth);

  Iteration* loop = current->AsIteration();
  __ jmp(loop->continue_target());
}


void FullCodeGenerator::VisitBreakStatement(BreakStatement* stmt) {
  Comment cmnt(masm_,  "[ BreakStatement");
  SetStatementPosition(stmt);
  NestedStatement* current = nesting_stack_;
  int stack_depth = 0;
  while (!current->IsBreakTarget(stmt->target())) {
    stack_depth = current->Exit(stack_depth);
    current = current->outer();
  }
  __ Drop(stack_depth);

  Breakable* target = current->AsBreakable();
  __ jmp(target->break_target());
}


void FullCodeGenerator::VisitReturnStatement(ReturnStatement* stmt) {
  Comment cmnt(masm_, "[ ReturnStatement");
  SetStatementPosition(stmt);
  Expression* expr = stmt->expression();
  VisitForValue(expr, kAccumulator);

  // Exit all nested statements.
  NestedStatement* current = nesting_stack_;
  int stack_depth = 0;
  while (current != NULL) {
    stack_depth = current->Exit(stack_depth);
    current = current->outer();
  }
  __ Drop(stack_depth);

  EmitReturnSequence(stmt->statement_pos());
}


void FullCodeGenerator::VisitWithEnterStatement(WithEnterStatement* stmt) {
  Comment cmnt(masm_, "[ WithEnterStatement");
  SetStatementPosition(stmt);

  VisitForValue(stmt->expression(), kStack);
  if (stmt->is_catch_block()) {
    __ CallRuntime(Runtime::kPushCatchContext, 1);
  } else {
    __ CallRuntime(Runtime::kPushContext, 1);
  }
  // Both runtime calls return the new context in both the context and the
  // result registers.

  // Update local stack frame context field.
  StoreToFrameField(StandardFrameConstants::kContextOffset, context_register());
}


void FullCodeGenerator::VisitWithExitStatement(WithExitStatement* stmt) {
  Comment cmnt(masm_, "[ WithExitStatement");
  SetStatementPosition(stmt);

  // Pop context.
  LoadContextField(context_register(), Context::PREVIOUS_INDEX);
  // Update local stack frame context field.
  StoreToFrameField(StandardFrameConstants::kContextOffset, context_register());
}


void FullCodeGenerator::VisitSwitchStatement(SwitchStatement* stmt) {
  UNREACHABLE();
}


void FullCodeGenerator::VisitDoWhileStatement(DoWhileStatement* stmt) {
  Comment cmnt(masm_, "[ DoWhileStatement");
  SetStatementPosition(stmt);
  Label body, stack_limit_hit, stack_check_success;

  Iteration loop_statement(this, stmt);
  increment_loop_depth();

  __ bind(&body);
  Visit(stmt->body());

  // Check stack before looping.
  __ StackLimitCheck(&stack_limit_hit);
  __ bind(&stack_check_success);

  __ bind(loop_statement.continue_target());
  SetStatementPosition(stmt->condition_position());
  VisitForControl(stmt->cond(), &body, loop_statement.break_target());

  __ bind(&stack_limit_hit);
  StackCheckStub stack_stub;
  __ CallStub(&stack_stub);
  __ jmp(&stack_check_success);

  __ bind(loop_statement.break_target());

  decrement_loop_depth();
}


void FullCodeGenerator::VisitWhileStatement(WhileStatement* stmt) {
  Comment cmnt(masm_, "[ WhileStatement");
  SetStatementPosition(stmt);
  Label body, stack_limit_hit, stack_check_success;

  Iteration loop_statement(this, stmt);
  increment_loop_depth();

  // Emit the test at the bottom of the loop.
  __ jmp(loop_statement.continue_target());

  __ bind(&body);
  Visit(stmt->body());

  __ bind(loop_statement.continue_target());
  // Check stack before looping.
  __ StackLimitCheck(&stack_limit_hit);
  __ bind(&stack_check_success);

  VisitForControl(stmt->cond(), &body, loop_statement.break_target());

  __ bind(&stack_limit_hit);
  StackCheckStub stack_stub;
  __ CallStub(&stack_stub);
  __ jmp(&stack_check_success);

  __ bind(loop_statement.break_target());
  decrement_loop_depth();
}


void FullCodeGenerator::VisitForStatement(ForStatement* stmt) {
  Comment cmnt(masm_, "[ ForStatement");
  SetStatementPosition(stmt);
  Label test, body, stack_limit_hit, stack_check_success;

  Iteration loop_statement(this, stmt);
  if (stmt->init() != NULL) {
    Visit(stmt->init());
  }

  increment_loop_depth();
  // Emit the test at the bottom of the loop (even if empty).
  __ jmp(&test);

  __ bind(&body);
  Visit(stmt->body());

  __ bind(loop_statement.continue_target());

  SetStatementPosition(stmt);
  if (stmt->next() != NULL) {
    Visit(stmt->next());
  }

  __ bind(&test);

  // Check stack before looping.
  __ StackLimitCheck(&stack_limit_hit);
  __ bind(&stack_check_success);

  if (stmt->cond() != NULL) {
    VisitForControl(stmt->cond(), &body, loop_statement.break_target());
  } else {
    __ jmp(&body);
  }

  __ bind(&stack_limit_hit);
  StackCheckStub stack_stub;
  __ CallStub(&stack_stub);
  __ jmp(&stack_check_success);

  __ bind(loop_statement.break_target());
  decrement_loop_depth();
}


void FullCodeGenerator::VisitForInStatement(ForInStatement* stmt) {
  UNREACHABLE();
}


void FullCodeGenerator::VisitTryCatchStatement(TryCatchStatement* stmt) {
  Comment cmnt(masm_, "[ TryCatchStatement");
  SetStatementPosition(stmt);
  // The try block adds a handler to the exception handler chain
  // before entering, and removes it again when exiting normally.
  // If an exception is thrown during execution of the try block,
  // control is passed to the handler, which also consumes the handler.
  // At this point, the exception is in a register, and store it in
  // the temporary local variable (prints as ".catch-var") before
  // executing the catch block. The catch block has been rewritten
  // to introduce a new scope to bind the catch variable and to remove
  // that scope again afterwards.

  Label try_handler_setup, catch_entry, done;
  __ Call(&try_handler_setup);
  // Try handler code, exception in result register.

  // Store exception in local .catch variable before executing catch block.
  {
    // The catch variable is *always* a variable proxy for a local variable.
    Variable* catch_var = stmt->catch_var()->AsVariableProxy()->AsVariable();
    ASSERT_NOT_NULL(catch_var);
    Slot* variable_slot = catch_var->slot();
    ASSERT_NOT_NULL(variable_slot);
    ASSERT_EQ(Slot::LOCAL, variable_slot->type());
    StoreToFrameField(SlotOffset(variable_slot), result_register());
  }

  Visit(stmt->catch_block());
  __ jmp(&done);

  // Try block code. Sets up the exception handler chain.
  __ bind(&try_handler_setup);
  {
    TryCatch try_block(this, &catch_entry);
    __ PushTryHandler(IN_JAVASCRIPT, TRY_CATCH_HANDLER);
    Visit(stmt->try_block());
    __ PopTryHandler();
  }
  __ bind(&done);
}


void FullCodeGenerator::VisitTryFinallyStatement(TryFinallyStatement* stmt) {
  Comment cmnt(masm_, "[ TryFinallyStatement");
  SetStatementPosition(stmt);
  // Try finally is compiled by setting up a try-handler on the stack while
  // executing the try body, and removing it again afterwards.
  //
  // The try-finally construct can enter the finally block in three ways:
  // 1. By exiting the try-block normally. This removes the try-handler and
  //      calls the finally block code before continuing.
  // 2. By exiting the try-block with a function-local control flow transfer
  //    (break/continue/return). The site of the, e.g., break removes the
  //    try handler and calls the finally block code before continuing
  //    its outward control transfer.
  // 3. by exiting the try-block with a thrown exception.
  //    This can happen in nested function calls. It traverses the try-handler
  //    chain and consumes the try-handler entry before jumping to the
  //    handler code. The handler code then calls the finally-block before
  //    rethrowing the exception.
  //
  // The finally block must assume a return address on top of the stack
  // (or in the link register on ARM chips) and a value (return value or
  // exception) in the result register (rax/eax/r0), both of which must
  // be preserved. The return address isn't GC-safe, so it should be
  // cooked before GC.
  Label finally_entry;
  Label try_handler_setup;

  // Setup the try-handler chain. Use a call to
  // Jump to try-handler setup and try-block code. Use call to put try-handler
  // address on stack.
  __ Call(&try_handler_setup);
  // Try handler code. Return address of call is pushed on handler stack.
  {
    // This code is only executed during stack-handler traversal when an
    // exception is thrown. The execption is in the result register, which
    // is retained by the finally block.
    // Call the finally block and then rethrow the exception.
    __ Call(&finally_entry);
    __ push(result_register());
    __ CallRuntime(Runtime::kReThrow, 1);
  }

  __ bind(&finally_entry);
  {
    // Finally block implementation.
    Finally finally_block(this);
    EnterFinallyBlock();
    Visit(stmt->finally_block());
    ExitFinallyBlock();  // Return to the calling code.
  }

  __ bind(&try_handler_setup);
  {
    // Setup try handler (stack pointer registers).
    TryFinally try_block(this, &finally_entry);
    __ PushTryHandler(IN_JAVASCRIPT, TRY_FINALLY_HANDLER);
    Visit(stmt->try_block());
    __ PopTryHandler();
  }
  // Execute the finally block on the way out.
  __ Call(&finally_entry);
}


void FullCodeGenerator::VisitDebuggerStatement(DebuggerStatement* stmt) {
#ifdef ENABLE_DEBUGGER_SUPPORT
  Comment cmnt(masm_, "[ DebuggerStatement");
  SetStatementPosition(stmt);
  __ CallRuntime(Runtime::kDebugBreak, 0);
  // Ignore the return value.
#endif
}


void FullCodeGenerator::VisitFunctionBoilerplateLiteral(
    FunctionBoilerplateLiteral* expr) {
  UNREACHABLE();
}


void FullCodeGenerator::VisitConditional(Conditional* expr) {
  Comment cmnt(masm_, "[ Conditional");
  Label true_case, false_case, done;
  VisitForControl(expr->condition(), &true_case, &false_case);

  __ bind(&true_case);
  Visit(expr->then_expression());
  // If control flow falls through Visit, jump to done.
  if (context_ == Expression::kEffect || context_ == Expression::kValue) {
    __ jmp(&done);
  }

  __ bind(&false_case);
  Visit(expr->else_expression());
  // If control flow falls through Visit, merge it with true case here.
  if (context_ == Expression::kEffect || context_ == Expression::kValue) {
    __ bind(&done);
  }
}


void FullCodeGenerator::VisitSlot(Slot* expr) {
  // Slots do not appear directly in the AST.
  UNREACHABLE();
}


void FullCodeGenerator::VisitLiteral(Literal* expr) {
  Comment cmnt(masm_, "[ Literal");
  Apply(context_, expr);
}


void FullCodeGenerator::VisitAssignment(Assignment* expr) {
  Comment cmnt(masm_, "[ Assignment");
  // Left-hand side can only be a property, a global or a (parameter or local)
  // slot. Variables with rewrite to .arguments are treated as KEYED_PROPERTY.
  enum LhsKind { VARIABLE, NAMED_PROPERTY, KEYED_PROPERTY };
  LhsKind assign_type = VARIABLE;
  Property* prop = expr->target()->AsProperty();
  if (prop != NULL) {
    assign_type =
        (prop->key()->IsPropertyName()) ? NAMED_PROPERTY : KEYED_PROPERTY;
  }

  // Evaluate LHS expression.
  switch (assign_type) {
    case VARIABLE:
      // Nothing to do here.
      break;
    case NAMED_PROPERTY:
      VisitForValue(prop->obj(), kStack);
      break;
    case KEYED_PROPERTY:
      VisitForValue(prop->obj(), kStack);
      VisitForValue(prop->key(), kStack);
      break;
  }

  // If we have a compound assignment: Get value of LHS expression and
  // store in on top of the stack.
  if (expr->is_compound()) {
    Location saved_location = location_;
    location_ = kStack;
    switch (assign_type) {
      case VARIABLE:
        EmitVariableLoad(expr->target()->AsVariableProxy()->var(),
                         Expression::kValue);
        break;
      case NAMED_PROPERTY:
        EmitNamedPropertyLoad(prop);
        __ push(result_register());
        break;
      case KEYED_PROPERTY:
        EmitKeyedPropertyLoad(prop);
        __ push(result_register());
        break;
    }
    location_ = saved_location;
  }

  // Evaluate RHS expression.
  Expression* rhs = expr->value();
  VisitForValue(rhs, kAccumulator);

  // If we have a compount assignment: Apply operator.
  if (expr->is_compound()) {
    Location saved_location = location_;
    location_ = kAccumulator;
    EmitBinaryOp(expr->binary_op(), Expression::kValue);
    location_ = saved_location;
  }

  // Record source position before possible IC call.
  SetSourcePosition(expr->position());

  // Store the value.
  switch (assign_type) {
    case VARIABLE:
      EmitVariableAssignment(expr->target()->AsVariableProxy()->var(),
                             context_);
      break;
    case NAMED_PROPERTY:
      EmitNamedPropertyAssignment(expr);
      break;
    case KEYED_PROPERTY:
      EmitKeyedPropertyAssignment(expr);
      break;
  }
}


void FullCodeGenerator::VisitCatchExtensionObject(CatchExtensionObject* expr) {
  // Call runtime routine to allocate the catch extension object and
  // assign the exception value to the catch variable.
  Comment cmnt(masm_, "[ CatchExtensionObject");
  VisitForValue(expr->key(), kStack);
  VisitForValue(expr->value(), kStack);
  // Create catch extension object.
  __ CallRuntime(Runtime::kCreateCatchExtensionObject, 2);
  Apply(context_, result_register());
}


void FullCodeGenerator::VisitThrow(Throw* expr) {
  Comment cmnt(masm_, "[ Throw");
  VisitForValue(expr->exception(), kStack);
  __ CallRuntime(Runtime::kThrow, 1);
  // Never returns here.
}


int FullCodeGenerator::TryFinally::Exit(int stack_depth) {
  // The macros used here must preserve the result register.
  __ Drop(stack_depth);
  __ PopTryHandler();
  __ Call(finally_entry_);
  return 0;
}


int FullCodeGenerator::TryCatch::Exit(int stack_depth) {
  // The macros used here must preserve the result register.
  __ Drop(stack_depth);
  __ PopTryHandler();
  return 0;
}


#undef __


} }  // namespace v8::internal
