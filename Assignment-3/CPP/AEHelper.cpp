//===- AEHelper.cpp -- Abstract Interpretation harness --//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2022>  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
/*
 * Harness for Assignment-3 abstract interpretation.
 *
 * Owns the harness-side `AEHelper::*` methods:
 *   - Analysis driver                     (runOnModule / analyse)
 *   - Interprocedural WTO construction   (initWTO)
 *   - Stub / checkpoint sub-dispatch     (handleStubFunctions /
 *                                         handleCheckpointStubs) — invoked
 *                                         from the student's handleCallSite
 *                                         in Assignment_3.cpp.
 *   - External-API whitelist             (isExternalCallForAssignment)
 *   - Abstract-state trace access        (getAEState / postAbsTrace)
 *   - Validator                          (ensureAllAssertsValidated)
 *
 * Pure bug-reporting concerns (AEReporter class + JSON / coverage summary)
 * live in AEReporter.cpp. The six assignment features live in
 * Assignment_3.cpp.
 */

#include "AEHelper.h"
#include "WPA/Andersen.h"
#include <cassert>
#include <sstream>

using namespace SVF;

void AEHelper::runOnModule(SVF::ICFG* moduleICFG) {
	svfir = PAG::getPAG();
	icfg = moduleICFG;
	analyse();
	if (!getReporter().getCaseConfig().emitJson)
		getReporter().printReport();
}

void AEHelper::analyse() {
	initWTO();
	handleGlobalNode();

	if (const FunObjVar* fun = svfir->getFunObjVar("main")) {
		for (u32_t i = 0; i < fun->arg_size(); ++i) {
			AEState& as = getAEState(icfg->getGlobalICFGNode());
			as[fun->getArg(i)->getId()] = IntervalValue::top();
		}
		assert(svfir->getFunObjVar("main") != nullptr && "Main function not found");
		handleFunction(icfg->getFunEntryICFGNode(fun));
	}
}

void AEHelper::reportBufOverflow(const ICFGNode* node) {
	AEException bug(node->toString());
	getReporter().addBugToReporter("buffer-overflow", bug, node);
}

void AEHelper::reportNullDeref(const ICFGNode* node) {
	AEException bug(node->toString());
	getReporter().addBugToReporter("nullptr-deref", bug, node);
}

/// Whitelist of external-call names covered by the assignment. Covers:
///   - Assignment-specific stubs:        `mem_insert`, `str_insert`
///   - Memory family:                    `memcpy`, `memmove`, `memset`
///   - String family:                    `strcpy`, `strncpy`, `strcat`,
///                                       `strncat`, `strlen`, `wcslen`
///   - Ground-truth checkpoint stubs:    `SAFE_/UNSAFE_BUFACCESS`,
///                                       `SAFE_/UNSAFE_PTRDEREF`
///
/// The library APIs are matched by substring because Clang emits the memory
/// family as LLVM intrinsics (e.g. `llvm.memcpy.p0.p0.i64`) and the substring
/// is preserved in the mangled name.
bool AEHelper::isExternalCallForAssignment(const SVF::FunObjVar* func) {
	const std::string& name = func->getName();
	static const Set<std::string> exactStubs = {
	    "mem_insert", "str_insert",
	    "UNSAFE_BUFACCESS", "SAFE_BUFACCESS",
	    "UNSAFE_PTRDEREF", "SAFE_PTRDEREF"};
	if (exactStubs.count(name))
		return true;
	static const std::vector<std::string> apiSubstrings = {
	    "memcpy", "memmove", "memset",
	    "strcpy", "strncpy", "strcat", "strncat",
	    "strlen", "wcslen"};
	for (const auto& key : apiSubstrings) {
		if (name.find(key) != std::string::npos)
			return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// WTO construction.  Each (mutually) recursive function's entry node becomes
// a WTO cycle head because intra-SCC call edges are turned into back-edges.
// The same widening/narrowing machinery used for loops then drives recursion
// to a fixpoint via handleICFGCycle; recursive callsites are filtered out in
// handleCallSite via `inSameCallGraphSCC`.
// ---------------------------------------------------------------------------

void AEHelper::initWTO() {
	ander = AndersenWaveDiff::createAndersenWaveDiff(svfir);
	Andersen::CallGraphSCC* callGraphScc = ander->getCallGraphSCC();
	callGraphScc->find();
	auto callGraph = ander->getCallGraph();

	for (auto it = callGraph->begin(); it != callGraph->end(); ++it) {
		const FunObjVar* fun = it->second->getFunction();
		if (fun->isDeclaration())
			continue;

		NodeID repNodeId = callGraphScc->repNode(it->second->getId());
		const NodeBS& cgSCCNodes = callGraphScc->subNodes(repNodeId);

		bool isEntry = it->second->getInEdges().empty();
		for (auto inEdge : it->second->getInEdges())
			if (!cgSCCNodes.test(inEdge->getSrcID()))
				isEntry = true;
		if (!isEntry)
			continue;

		Set<const FunObjVar*> funcScc;
		for (const auto& node : cgSCCNodes)
			funcScc.insert(callGraph->getGNode(node)->getFunction());

		auto* wto = new ICFGWTO(icfg->getFunEntryICFGNode(fun), funcScc);
		wto->init();
		funcToWTO[fun] = wto;
	}
}

/// Verify that every ground-truth stub call site was reached by the student's
/// analysis (added to `assert_points` via handleCallSite -> handleStubFunctions
/// / handleCheckpointStubs).  A missed stub site means the student's
/// control-flow logic skipped a place the grader cares about.
///
/// Recognised stubs:
///   - svf_assert / svf_assert_eq         : abstract-state assertion checks
///   - UNSAFE_PTRDEREF / SAFE_PTRDEREF    : null-deref ground truth
///   - UNSAFE_BUFACCESS / SAFE_BUFACCESS  : buffer-access ground truth
///
void AEHelper::ensureAllAssertsValidated() {
	static const Set<std::string> kAssertStubs = {"svf_assert", "svf_assert_eq"};
	static const Set<std::string> kCheckpointStubs = {
	    "UNSAFE_PTRDEREF", "SAFE_PTRDEREF",
	    "UNSAFE_BUFACCESS", "SAFE_BUFACCESS"};
	Set<std::string> checkpointKinds;
	Map<std::string, u32_t> expectedReports;
	for (auto it = svfir->getICFG()->begin(); it != svfir->getICFG()->end(); ++it) {
		const ICFGNode* node = it->second;
		const CallICFGNode* call = SVFUtil::dyn_cast<CallICFGNode>(node);
		if (!call)
			continue;
		const FunObjVar* fun = call->getCalledFunction();
		if (!fun)
			continue;
		const std::string& name = fun->getName();
		const bool isAssertStub = kAssertStubs.count(name) > 0;
		const bool isCheckpointStub = kCheckpointStubs.count(name) > 0;
		if (!isAssertStub && !isCheckpointStub)
			continue;
		if (!bugReporter.isAssertionPoint(call)) {
			std::stringstream ss;
			ss << "The stub function callsite (" << name
			   << ") was not reached by the student's control flow: "
			   << call->toString();
			std::cerr << ss.str() << std::endl;
			assert(false);
		}
		if (isCheckpointStub) {
			const std::string kind =
			    name.find("BUFACCESS") != std::string::npos
			        ? "buffer-overflow"
			        : "nullptr-deref";
			checkpointKinds.insert(kind);
			if (name.rfind("UNSAFE_", 0) == 0)
				++expectedReports[kind];
		}
	}
	for (const std::string& kind : checkpointKinds) {
		u32_t actualReports = 0;
		for (const auto& report : bugReporter.getReports()) {
			if (report.kind == kind)
				++actualReports;
		}
		const u32_t expected = expectedReports[kind];
		assert(((expected == 0 && actualReports == 0) ||
		        (expected > 0 && actualReports >= expected)) &&
		       "SAFE/UNSAFE checkpoint report count mismatch");
	}
}

// ---------------------------------------------------------------------------
// Legacy fixture predicates. Checkpoint grading deliberately does not use
// these to create reports; reports must come from the student's checker.
// ---------------------------------------------------------------------------

namespace {
bool harnessSafeAccess(AbstractState& as, SVFIR* svfir, const ValVar* value,
                       const IntervalValue& len) {
	AbstractValue ptrVal = as[value->getId()];
	if (!ptrVal.isAddr())
		return true;
	for (const auto& addr : ptrVal.getAddrs()) {
		if (AbstractState::isBlackHoleObjAddr(addr) || AbstractState::isNullMem(addr))
			continue;
		NodeID objId = as.getIDFromAddr(addr);
		const BaseObjVar* baseObj = svfir->getBaseObject(objId);
		if (!baseObj || baseObj->isBlackHoleObj() || !baseObj->isConstantByteSize())
			continue;
		u32_t size = baseObj->getByteSizeOfObj();
		IntervalValue baseOffset(0);
		const SVFVar* svfVar = svfir->getGNode(objId);
		if (auto* gepObj = SVFUtil::dyn_cast<GepObjVar>(svfVar))
			baseOffset = IntervalValue((s64_t)gepObj->getConstantFieldIdx());
		IntervalValue offset = baseOffset + len;
		if (offset.ub().getIntNumeral() >= (s64_t)size)
			return false;
	}
	return true;
}

bool harnessSafeDeref(AbstractState& as, const ValVar* value) {
	if (!value || value->getId() == IRGraph::NullPtr)
		return false;
	const AbstractValue& absVal = as[value->getId()];
	if (!absVal.isAddr())
		return true;
	for (const auto& addr : absVal.getAddrs()) {
		if (AbstractState::isBlackHoleObjAddr(addr))
			continue;
		if (AbstractState::isNullMem(addr))
			return false;
		if (as.isFreedMem(addr))
			return false;
	}
	return true;
}
} // namespace

/// Record that the student's control flow reached a SAFE/UNSAFE checkpoint.
void AEHelper::handleCheckpointStubs(const CallICFGNode* callNode) {
	bugReporter.noteAssertionPoint(callNode);
}

/// Handle the abstract-state assertion stubs.  `svf_assert(expr)` requires the
/// expression to hold true; `svf_assert_eq(a, b)` requires the two intervals
/// to be equal.  Both record the call site in `assert_points` so
/// `ensureAllAssertsValidated` can verify coverage.
void AEHelper::handleStubFunctions(const SVF::CallICFGNode* callNode) {
	if (callNode->getCalledFunction()->getName() == "svf_assert") {
		bugReporter.noteAssertionPoint(callNode);
		u32_t arg0 = callNode->getArgument(0)->getId();
		AEState& as = getAEState(callNode);

		if (as[arg0].getInterval().is_infinite()) {
			SVFUtil::errs() << "svf_assert Fail. " << callNode->toString() << "\n";
			assert(false);
		}
		else {
			if (as[arg0].getInterval().equals(IntervalValue(1, 1))) {
				std::stringstream ss;
				ss << "The assertion (" << callNode->toString() << ")"
				   << " is successfully verified!!\n";
				SVFUtil::outs() << ss.str() << std::endl;
			}
			else {
				std::stringstream ss;
				ss << "The assertion (" << callNode->toString() << ")"
				   << " is unsatisfiable!!\n";
				SVFUtil::outs() << ss.str() << std::endl;
				assert(false);
			}
		}
		return;
	}
	else if (callNode->getCalledFunction()->getName() == "svf_assert_eq")  {
		bugReporter.noteAssertionPoint(callNode);
		u32_t arg0 = callNode->getArgument(0)->getId();
		u32_t arg1 = callNode->getArgument(1)->getId();
		AEState& as = getAEState(callNode);
		if (as[arg0].getInterval().equals(as[arg1].getInterval())) {
			SVFUtil::errs() << SVFUtil::sucMsg("The assertion is successfully verified!!\n");
		}
		else {
			SVFUtil::errs() << "svf_assert_eq Fail. " << callNode->toString() << "\n";
			assert(false);
		}
		return;
	}
}

// ===========================================================================
// Assignment-3 state and call helpers.
// ===========================================================================
namespace SVF {

/// CallPE is phi-like: the formal parameter joins the caller-side value from
/// every call site represented by the statement.
void AEHelper::updateStateOnCall(const CallPE* callPE) {
	AEState& state = getAEState(callPE->getICFGNode());
	AbstractValue joined;
	for (u32_t index = 0; index < callPE->getOpVarNum(); ++index) {
		const ICFGNode* callNode = callPE->getOpCallICFGNode(index);
		if (postAbsTrace.count(callNode))
			joined.join_with(postAbsTrace[callNode][callPE->getOpVarID(index)]);
	}
	state[callPE->getResID()] = joined;
}

// Assignment-3-owned post-trace accessors.
AEState& AEHelper::getAEState(const ICFGNode* node) {
	return postAbsTrace[node];
}

} // namespace SVF
