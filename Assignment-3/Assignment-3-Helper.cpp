#include "Assignment-3.h"
#include "SVFIR/SVFIR.h"
using namespace SVF;

// according to varieties of cmp insts,
// maybe var X var, var X const, const X var, const X const
// we accept 'var X const' 'var X var' 'const X const'
// if 'const X var', we need to reverse op0 op1 and its predicate 'var X' const'
// X' is reverse predicate of X
// == -> !=, != -> ==, > -> <=, >= -> <, < -> >=, <= -> >
Map<s32_t, s32_t> _reverse_predicate = {
    {CmpStmt::Predicate::FCMP_OEQ, CmpStmt::Predicate::FCMP_ONE}, // == -> !=
    {CmpStmt::Predicate::FCMP_UEQ, CmpStmt::Predicate::FCMP_UNE}, // == -> !=
    {CmpStmt::Predicate::FCMP_OGT, CmpStmt::Predicate::FCMP_OLE}, // > -> <=
    {CmpStmt::Predicate::FCMP_OGE, CmpStmt::Predicate::FCMP_OLT}, // >= -> <
    {CmpStmt::Predicate::FCMP_OLT, CmpStmt::Predicate::FCMP_OGE}, // < -> >=
    {CmpStmt::Predicate::FCMP_OLE, CmpStmt::Predicate::FCMP_OGT}, // <= -> >
    {CmpStmt::Predicate::FCMP_ONE, CmpStmt::Predicate::FCMP_OEQ}, // != -> ==
    {CmpStmt::Predicate::FCMP_UNE, CmpStmt::Predicate::FCMP_UEQ}, // != -> ==
    {CmpStmt::Predicate::ICMP_EQ, CmpStmt::Predicate::ICMP_NE}, // == -> !=
    {CmpStmt::Predicate::ICMP_NE, CmpStmt::Predicate::ICMP_EQ}, // != -> ==
    {CmpStmt::Predicate::ICMP_UGT, CmpStmt::Predicate::ICMP_ULE}, // > -> <=
    {CmpStmt::Predicate::ICMP_ULT, CmpStmt::Predicate::ICMP_UGE}, // < -> >=
    {CmpStmt::Predicate::ICMP_UGE, CmpStmt::Predicate::ICMP_ULT}, // >= -> <
    {CmpStmt::Predicate::ICMP_SGT, CmpStmt::Predicate::ICMP_SLE}, // > -> <=
    {CmpStmt::Predicate::ICMP_SLT, CmpStmt::Predicate::ICMP_SGE}, // < -> >=
    {CmpStmt::Predicate::ICMP_SGE, CmpStmt::Predicate::ICMP_SLT}, // >= -> <
};

Map<s32_t, s32_t> _switch_lhsrhs_predicate = {
    {CmpStmt::Predicate::FCMP_OEQ, CmpStmt::Predicate::FCMP_OEQ}, // == -> ==
    {CmpStmt::Predicate::FCMP_UEQ, CmpStmt::Predicate::FCMP_UEQ}, // == -> ==
    {CmpStmt::Predicate::FCMP_OGT, CmpStmt::Predicate::FCMP_OLT}, // > -> <
    {CmpStmt::Predicate::FCMP_OGE, CmpStmt::Predicate::FCMP_OLE}, // >= -> <=
    {CmpStmt::Predicate::FCMP_OLT, CmpStmt::Predicate::FCMP_OGT}, // < -> >
    {CmpStmt::Predicate::FCMP_OLE, CmpStmt::Predicate::FCMP_OGE}, // <= -> >=
    {CmpStmt::Predicate::FCMP_ONE, CmpStmt::Predicate::FCMP_ONE}, // != -> !=
    {CmpStmt::Predicate::FCMP_UNE, CmpStmt::Predicate::FCMP_UNE}, // != -> !=
    {CmpStmt::Predicate::ICMP_EQ, CmpStmt::Predicate::ICMP_EQ}, // == -> ==
    {CmpStmt::Predicate::ICMP_NE, CmpStmt::Predicate::ICMP_NE}, // != -> !=
    {CmpStmt::Predicate::ICMP_UGT, CmpStmt::Predicate::ICMP_ULT}, // > -> <
    {CmpStmt::Predicate::ICMP_ULT, CmpStmt::Predicate::ICMP_UGT}, // < -> >
    {CmpStmt::Predicate::ICMP_UGE, CmpStmt::Predicate::ICMP_ULE}, // >= -> <=
    {CmpStmt::Predicate::ICMP_SGT, CmpStmt::Predicate::ICMP_SLT}, // > -> <
    {CmpStmt::Predicate::ICMP_SLT, CmpStmt::Predicate::ICMP_SGT}, // < -> >
    {CmpStmt::Predicate::ICMP_SGE, CmpStmt::Predicate::ICMP_SLE}, // >= -> <=
};

/**
 * @brief Initialize an object variable in the abstract state
 *
 * This function initializes an object variable in the given abstract state. It sets the initial value
 * based on the characteristics of the memory object associated with the variable. It handles constant data,
 * constant arrays, constant structures, and other types appropriately.
 *
 * @param as The abstract state to be updated
 * @param objVar The object variable to be initialized
 */
void AbstractExecution::initObjVar(AbstractState& as, ObjVar* objVar) {
	NodeID varId = objVar->getId();

	// Check if the object variable has an associated value
	if (objVar->hasValue()) {
		const MemObj* obj = objVar->getMemObj();

		// Handle constant data, arrays, and structures
		if (obj->isConstDataOrConstGlobal() || obj->isConstantArray() || obj->isConstantStruct()) {
			if (const SVFConstantInt* consInt = SVFUtil::dyn_cast<SVFConstantInt>(obj->getValue())) {
				s64_t numeral = consInt->getSExtValue();
				as[varId] = IntervalValue(numeral, numeral);
			}
			else if (const SVFConstantFP* consFP = SVFUtil::dyn_cast<SVFConstantFP>(obj->getValue())) {
				as[varId] = IntervalValue(consFP->getFPValue(), consFP->getFPValue());
			}
			else if (SVFUtil::isa<SVFConstantNullPtr>(obj->getValue())) {
				as[varId] = IntervalValue(0, 0);
			}
			else if (SVFUtil::isa<SVFGlobalValue>(obj->getValue())) {
				as[varId] = AddressValue(AbstractState::getVirtualMemAddress(varId));
			}
			else if (obj->isConstantArray() || obj->isConstantStruct()) {
				as[varId] = IntervalValue::top();
			}
			else {
				as[varId] = IntervalValue::top();
			}
		}
		// Handle non-constant memory objects
		else {
			as[varId] = AddressValue(AbstractState::getVirtualMemAddress(varId));
		}
	}
	// If the object variable does not have an associated value, set it to a virtual memory address
	else {
		as[varId] = AddressValue(AbstractState::getVirtualMemAddress(varId));
	}

	return;
}

void AbstractExecution::runOnModule(SVF::ICFG* icfg) {
	_svfir = PAG::getPAG();
	_ander = AndersenWaveDiff::createAndersenWaveDiff(_svfir);
	_icfg = icfg;
	analyse();
	_bufOverflowHelper.printReport();
}

/**
 * @brief Mark recursive functions in the call graph
 *
 * This function identifies and marks recursive functions in the call graph.
 * It does this by detecting cycles in the call graph's strongly connected components (SCC).
 * Any function found to be part of a cycle is marked as recursive.
 */
void AbstractExecution::initWTO() {
	// Detect if the call graph has cycles by finding its strongly connected components (SCC)
	Andersen::CallGraphSCC* callGraphScc = _ander->getCallGraphSCC();
	callGraphScc->find();
	auto callGraph = _ander->getPTACallGraph();

	// Iterate through the call graph
	for (auto it = callGraph->begin(); it != callGraph->end(); it++) {
		// Check if the current function is part of a cycle
		if (callGraphScc->isInCycle(it->second->getId()))
			_recursiveFuns.insert(it->second->getFunction()); // Mark the function as recursive
	}

	// Initialize WTO for each function in the module
	for (const SVFFunction* fun : _svfir->getModule()->getFunctionSet()) {
		auto* wto = new ICFGWTO(_icfg, _icfg->getFunEntryICFGNode(fun));
		wto->init();
		_funcToWTO[fun] = wto;
	}
}

/**
 * @brief Get the address value for a GEP (GetElementPtr) object
 *
 * This function calculates the address value for a GEP object given the abstract state, a pointer, and an offset.
 * It processes the addresses associated with the pointer and adjusts them based on the given offset.
 *
 * @param as The current abstract state
 * @param pointer The pointer to the base address
 * @param offset The offset to be applied to the base address
 * @return AddressValue The calculated address value for the GEP object
 */
AddressValue AbstractExecution::getGepObjAddress(AbstractState& as, u32_t pointer, APOffset offset) {
	AbstractValue addrs = as[pointer];
	AddressValue ret = AddressValue();
	for (const auto& addr : addrs.getAddrs()) {
		s64_t baseObj = AbstractState::getInternalID(addr);
		assert(SVFUtil::isa<ObjVar>(_svfir->getGNode(baseObj)) && "Fail to get the base object address!");
		NodeID gepObj = _svfir->getGepObjVar(baseObj, offset);
		as[gepObj] = AddressValue(AbstractState::getVirtualMemAddress(gepObj));
		ret.insert(AbstractState::getVirtualMemAddress(gepObj));
	}
	return ret;
}

/**
 * @brief Get the byte offset interval for a given abstract state and GEP statement
 *
 * This function calculates the byte offset interval for a given abstract state
 * and a GEP (GetElementPtr) statement. If the GEP statement has a constant byte offset,
 * it directly returns the interval value of that constant byte offset. Otherwise, it
 * computes the byte offset interval based on the list of offset variable and type pairs.
 * e.g. int arr[20]; int idx = 10; arr[idx] => arr[idx] is a GEP statement, idx is an offset variable,
 * call getByteOffset(as, gep) to get the byte offset interval for arr[idx] (10*4 = 40 Bytes, [40,40])
 *
 * @param as The abstract state, which contains a mapping of value nodes to their corresponding interval values
 * @param gep The GEP statement for which the byte offset is to be calculated
 * @return IntervalValue The interval value of the byte offset
 */
IntervalValue AbstractExecution::getByteOffset(const AbstractState& as, const GepStmt* gep) {
	// If the GEP statement has a constant byte offset, return it directly as the interval value
	if (gep->isConstantOffset())
		return IntervalValue((s64_t)gep->accumulateConstantByteOffset());

	IntervalValue res(0); // Initialize the result interval 'res' to 0.

	// Loop through the offsetVarAndGepTypePairVec in reverse order.
	for (int i = gep->getOffsetVarAndGepTypePairVec().size() - 1; i >= 0; i--) {
		const SVFVar* idxOperandVar = gep->getOffsetVarAndGepTypePairVec()[i].first;
		const SVFType* idxOperandType = gep->getOffsetVarAndGepTypePairVec()[i].second;

		// Calculate the byte offset for array or pointer types
		if (SVFUtil::isa<SVFArrayType>(idxOperandType) || SVFUtil::isa<SVFPointerType>(idxOperandType)) {
			u32_t elemByteSize = 1;
			if (const SVFArrayType* arrOperandType = SVFUtil::dyn_cast<SVFArrayType>(idxOperandType))
				elemByteSize = arrOperandType->getTypeOfElement()->getByteSize();
			else if (SVFUtil::isa<SVFPointerType>(idxOperandType))
				elemByteSize = gep->getAccessPath().gepSrcPointeeType()->getByteSize();
			else
				assert(false && "idxOperandType must be ArrType or PtrType");

			if (const SVFConstantInt* op = SVFUtil::dyn_cast<SVFConstantInt>(idxOperandVar->getValue())) {
				// Calculate the lower bound (lb) of the interval value
				s64_t lb = (double)Options::MaxFieldLimit() / elemByteSize >= op->getSExtValue()
				               ? op->getSExtValue() * elemByteSize
				               : Options::MaxFieldLimit();
				res = res + IntervalValue(lb, lb);
			}
			else {
				u32_t idx = _svfir->getValueNode(idxOperandVar->getValue());
				IntervalValue idxVal = as[idx].getInterval();

				if (idxVal.isBottom())
					res = res + IntervalValue(0, 0);
				else {
					// Ensure the bounds are non-negative and within the field limit
					s64_t ub = (idxVal.ub().getIntNumeral() < 0) ? 0
					           : (double)Options::MaxFieldLimit() / elemByteSize >= idxVal.ub().getIntNumeral()
					               ? elemByteSize * idxVal.ub().getIntNumeral()
					               : Options::MaxFieldLimit();
					s64_t lb = (idxVal.lb().getIntNumeral() < 0) ? 0
					           : (double)Options::MaxFieldLimit() / elemByteSize >= idxVal.lb().getIntNumeral()
					               ? elemByteSize * idxVal.lb().getIntNumeral()
					               : Options::MaxFieldLimit();
					res = res + IntervalValue(lb, ub);
				}
			}
		}
		// Process struct subtypes by calculating the byte offset from the beginning to the field of the struct
		else if (const SVFStructType* structOperandType = SVFUtil::dyn_cast<SVFStructType>(idxOperandType)) {
			res = res + IntervalValue(gep->getAccessPath().getStructFieldOffset(idxOperandVar, structOperandType));
		}
		else {
			assert(false && "gep type pair only support arr/ptr/struct");
		}
	}
	return res; // Return the resulting byte offset as an IntervalValue.
}

/**
 * @brief Get the interval value of an element index for a given abstract state and GEP statement
 *
 * This function calculates the interval value of the element index for a given abstract state
 * and a GEP (GetElementPtr) statement. If the GEP statement has a constant offset, it directly
 * returns the interval value of that constant offset. Otherwise, it computes the interval value
 * based on the list of offset variable and type pairs.
 * e.g. int arr[20]; int idx = 10; arr[idx] => arr[idx] is a GEP statement, idx is an offset variable,
 * call this function with the abstract state and GEP statement, the function will return the interval of idx ([10,10]);
 *
 * @param as The abstract state, which contains a mapping of value nodes to their corresponding interval values
 * @param gep The GEP statement for which the element index is to be calculated
 * @return IntervalValue The interval value of the element index
 */
IntervalValue AbstractExecution::getElementIndex(const AbstractState& as, const GepStmt* gep) {
	// If the GEP statement has a constant offset, return it directly as the interval value
	if (gep->isConstantOffset())
		return IntervalValue((s64_t)gep->accumulateConstantOffset());

	IntervalValue res(0);
	// Iterate over the list of offset variable and type pairs in reverse order
	for (int i = gep->getOffsetVarAndGepTypePairVec().size() - 1; i >= 0; i--) {
		AccessPath::IdxOperandPair IdxVarAndType = gep->getOffsetVarAndGepTypePairVec()[i];
		const SVFValue* value = gep->getOffsetVarAndGepTypePairVec()[i].first->getValue();
		const SVFType* type = IdxVarAndType.second;

		// Variables to store the lower and upper bounds of the index value
		s64_t idxLb;
		s64_t idxUb;

		// Determine the lower and upper bounds based on whether the value is a constant
		if (const SVFConstantInt* constInt = SVFUtil::dyn_cast<SVFConstantInt>(value))
			idxLb = idxUb = constInt->getSExtValue();
		else {
			IntervalValue idxItv = as[_svfir->getValueNode(value)].getInterval();
			if (idxItv.isBottom())
				idxLb = idxUb = 0;
			else {
				idxLb = idxItv.lb().getIntNumeral();
				idxUb = idxItv.ub().getIntNumeral();
			}
		}

		// Adjust the bounds if the type is a pointer
		if (SVFUtil::isa<SVFPointerType>(type)) {
			u32_t elemNum = gep->getAccessPath().getElementNum(gep->getAccessPath().gepSrcPointeeType());
			idxLb = (double)Options::MaxFieldLimit() / elemNum < idxLb ? Options::MaxFieldLimit() : idxLb * elemNum;
			idxUb = (double)Options::MaxFieldLimit() / elemNum < idxUb ? Options::MaxFieldLimit() : idxUb * elemNum;
		}
		// Adjust the bounds for array or struct types using the symbol table info
		else {
			if (Options::ModelArrays()) {
				const std::vector<u32_t>& so = SymbolTableInfo::SymbolInfo()->getTypeInfo(type)->getFlattenedElemIdxVec();
				if (so.empty() || idxUb >= (APOffset)so.size() || idxLb < 0) {
					idxLb = idxUb = 0;
				}
				else {
					idxLb = SymbolTableInfo::SymbolInfo()->getFlattenedElemIdx(type, idxLb);
					idxUb = SymbolTableInfo::SymbolInfo()->getFlattenedElemIdx(type, idxUb);
				}
			}
			else
				idxLb = idxUb = 0;
		}

		// Add the calculated interval to the result
		res = res + IntervalValue(idxLb, idxUb);
	}

	// Ensure the result is within the bounds of [0, MaxFieldLimit]
	res.meet_with(IntervalValue((s64_t)0, (s64_t)Options::MaxFieldLimit()));
	if (res.isBottom()) {
		res = IntervalValue(0);
	}
	return res;
}

/**
 * @brief  Propagate the states from predecessors to the current node and return true if the control-flow is feasible
 *
 * This function attempts to propagate the execution state to a given block by merging the states
 * of its predecessor blocks. It handles two scenarios: intra-block edges and call edges.
 *  Scenario 1: preblock -----(intraEdge)----> block, join the preES of inEdges
 *  Scenario 2: preblock -----(callEdge)----> block
 * If the propagation is feasible, it updates the execution state and returns true. Otherwise, it returns false.
 *
 * @param block The ICFG node (block) for which the state propagation is attempted
 * @return bool True if the state propagation is feasible and successful, false otherwise
 */
bool AbstractExecution::mergeStatesFromPredecessors(const ICFGNode* block, AbstractState& as) {
	u32_t inEdgeNum = 0; // Initialize the number of incoming edges with feasible states

	// Iterate over all incoming edges of the given block
	for (auto& edge : block->getInEdges()) {
		// Check if the source node of the edge has a post-execution state recorded
		if (_postAbsTrace.find(edge->getSrcNode()) != _postAbsTrace.end()) {
			const IntraCFGEdge* intraCfgEdge = SVFUtil::dyn_cast<IntraCFGEdge>(edge);

			// If the edge is an intra-block edge and has a condition
			if (intraCfgEdge && intraCfgEdge->getCondition()) {
				AbstractState tmpEs = _postAbsTrace[edge->getSrcNode()];
				// Check if the branch condition is feasible
				if (isBranchFeasible(intraCfgEdge, tmpEs)) {
					as.joinWith(tmpEs); // Merge the state with the current state
					inEdgeNum++;
				}
				// If branch is not feasible, do nothing
			}
			else {
				// For non-conditional edges, directly merge the state
				as.joinWith(_postAbsTrace[edge->getSrcNode()]);
				inEdgeNum++;
			}
		}
		// If no post-execution state is recorded for the source node, do nothing
	}

	// If no incoming edges have feasible states, return false
	if (inEdgeNum == 0) {
		return false;
	}
	else {
		return true;
	}
	assert(false && "implement this part"); // This part should not be reached
}

bool AbstractExecution::isCmpBranchFeasible(const CmpStmt* cmpStmt, s64_t succ, AbstractState& as) {
	AbstractState new_es = as;
	// get cmp stmt's op0, op1, and predicate
	NodeID op0 = cmpStmt->getOpVarID(0);
	NodeID op1 = cmpStmt->getOpVarID(1);
	NodeID res_id = cmpStmt->getResID();
	s32_t predicate = cmpStmt->getPredicate();

	// if op0 or op1 is undefined, return;
	// skip address compare
	if (new_es.inVarToAddrsTable(op0) || new_es.inVarToAddrsTable(op1)) {
		as = new_es;
		return true;
	}
	// get '%1 = load i32 s', and load inst may not exist
	auto getLoadOp = [](SVFVar* opVar) -> const LoadStmt* {
		if (!opVar->getInEdges().empty()) {
			SVFStmt* loadVar0InStmt = *opVar->getInEdges().begin();
			if (const LoadStmt* loadStmt = SVFUtil::dyn_cast<LoadStmt>(loadVar0InStmt)) {
				return loadStmt;
			}
			else if (const CopyStmt* copyStmt = SVFUtil::dyn_cast<CopyStmt>(loadVar0InStmt)) {
				if (!copyStmt->getRHSVar()->getInEdges().empty()) {
					SVFStmt* loadVar0InStmt2 = *opVar->getInEdges().begin();
					if (const LoadStmt* loadStmt = SVFUtil::dyn_cast<LoadStmt>(loadVar0InStmt2)) {
						return loadStmt;
					}
				}
			}
		}
		return nullptr;
	};
	const LoadStmt* load_op0 = getLoadOp(_svfir->getGNode(op0));
	const LoadStmt* load_op1 = getLoadOp(_svfir->getGNode(op1));

	// for const X const, we may get concrete resVal instantly
	// for var X const, we may get [0,1] if the intersection of var and const is not empty set
	IntervalValue resVal = new_es[res_id].getInterval();
	resVal.meet_with(IntervalValue((s64_t)succ, succ));
	// If Var X const generates bottom value, it means this branch path is not feasible.
	if (resVal.isBottom()) {
		return false;
	}

	bool b0 = new_es[op0].getInterval().is_numeral();
	bool b1 = new_es[op1].getInterval().is_numeral();

	// if const X var, we should reverse op0 and op1.
	if (b0 && !b1) {
		std::swap(op0, op1);
		std::swap(load_op0, load_op1);
		predicate = _switch_lhsrhs_predicate[predicate];
	}
	else {
		// if var X var, we cannot preset the branch condition to infer the intervals of var0,var1
		if (!b0 && !b1) {
			as = new_es;
			return true;
		}
		// if const X const, we can instantly get the resVal
		else if (b0 && b1) {
			as = new_es;
			return true;
		}
	}
	// if cmp is 'var X const == false', we should reverse predicate 'var X' const == true'
	// X' is reverse predicate of X
	if (succ == 0) {
		predicate = _reverse_predicate[predicate];
	}
	else {
	}
	// change interval range according to the compare predicate
	AddressValue addrs;
	if (load_op0 && new_es.inVarToAddrsTable(load_op0->getRHSVarID()))
		addrs = new_es[load_op0->getRHSVarID()].getAddrs();

	IntervalValue &lhs = new_es[op0].getInterval(), &rhs = new_es[op1].getInterval();
	switch (predicate) {
	case CmpStmt::Predicate::ICMP_EQ:
	case CmpStmt::Predicate::FCMP_OEQ:
	case CmpStmt::Predicate::FCMP_UEQ: {
		// Var == Const, so [var.lb, var.ub].meet_with(const)
		lhs.meet_with(rhs);
		break;
	}
	case CmpStmt::Predicate::ICMP_NE:
	case CmpStmt::Predicate::FCMP_ONE:
	case CmpStmt::Predicate::FCMP_UNE:
		// Compliment set
		break;
	case CmpStmt::Predicate::ICMP_UGT:
	case CmpStmt::Predicate::ICMP_SGT:
	case CmpStmt::Predicate::FCMP_OGT:
	case CmpStmt::Predicate::FCMP_UGT:
		// Var > Const, so [var.lb, var.ub].meet_with([Const+1, +INF])
		lhs.meet_with(IntervalValue(rhs.lb() + 1, IntervalValue::plus_infinity()));
		break;
	case CmpStmt::Predicate::ICMP_UGE:
	case CmpStmt::Predicate::ICMP_SGE:
	case CmpStmt::Predicate::FCMP_OGE:
	case CmpStmt::Predicate::FCMP_UGE: {
		// Var >= Const, so [var.lb, var.ub].meet_with([Const, +INF])
		lhs.meet_with(IntervalValue(rhs.lb(), IntervalValue::plus_infinity()));
		break;
	}
	case CmpStmt::Predicate::ICMP_ULT:
	case CmpStmt::Predicate::ICMP_SLT:
	case CmpStmt::Predicate::FCMP_OLT:
	case CmpStmt::Predicate::FCMP_ULT: {
		// Var < Const, so [var.lb, var.ub].meet_with([-INF, const.ub-1])
		lhs.meet_with(IntervalValue(IntervalValue::minus_infinity(), rhs.ub() - 1));
		break;
	}
	case CmpStmt::Predicate::ICMP_ULE:
	case CmpStmt::Predicate::ICMP_SLE:
	case CmpStmt::Predicate::FCMP_OLE:
	case CmpStmt::Predicate::FCMP_ULE: {
		// Var <= Const, so [var.lb, var.ub].meet_with([-INF, const.ub])
		lhs.meet_with(IntervalValue(IntervalValue::minus_infinity(), rhs.ub()));
		break;
	}
	case CmpStmt::Predicate::FCMP_FALSE: break;
	case CmpStmt::Predicate::FCMP_TRUE: break;
	default: assert(false && "implement this part"); abort();
	}

	for (const auto& addr : addrs) {
		NodeID objId = SVF::AbstractState::getInternalID(addr);
		if (new_es.inAddrToValTable(objId)) {
			switch (predicate) {
			case CmpStmt::Predicate::ICMP_EQ:
			case CmpStmt::Predicate::FCMP_OEQ:
			case CmpStmt::Predicate::FCMP_UEQ: {
				new_es.load(addr).meet_with(rhs);
				break;
			}
			case CmpStmt::Predicate::ICMP_NE:
			case CmpStmt::Predicate::FCMP_ONE:
			case CmpStmt::Predicate::FCMP_UNE:
				// Compliment set
				break;
			case CmpStmt::Predicate::ICMP_UGT:
			case CmpStmt::Predicate::ICMP_SGT:
			case CmpStmt::Predicate::FCMP_OGT:
			case CmpStmt::Predicate::FCMP_UGT:
				// Var > Const, so [var.lb, var.ub].meet_with([Const+1, +INF])
				new_es.load(addr).meet_with(IntervalValue(rhs.lb() + 1, IntervalValue::plus_infinity()));
				break;
			case CmpStmt::Predicate::ICMP_UGE:
			case CmpStmt::Predicate::ICMP_SGE:
			case CmpStmt::Predicate::FCMP_OGE:
			case CmpStmt::Predicate::FCMP_UGE: {
				// Var >= Const, so [var.lb, var.ub].meet_with([Const, +INF])
				new_es.load(addr).meet_with(IntervalValue(rhs.lb(), IntervalValue::plus_infinity()));
				break;
			}
			case CmpStmt::Predicate::ICMP_ULT:
			case CmpStmt::Predicate::ICMP_SLT:
			case CmpStmt::Predicate::FCMP_OLT:
			case CmpStmt::Predicate::FCMP_ULT: {
				// Var < Const, so [var.lb, var.ub].meet_with([-INF, const.ub-1])
				new_es.load(addr).meet_with(IntervalValue(IntervalValue::minus_infinity(), rhs.ub() - 1));
				break;
			}
			case CmpStmt::Predicate::ICMP_ULE:
			case CmpStmt::Predicate::ICMP_SLE:
			case CmpStmt::Predicate::FCMP_OLE:
			case CmpStmt::Predicate::FCMP_ULE: {
				// Var <= Const, so [var.lb, var.ub].meet_with([-INF, const.ub])
				new_es.load(addr).meet_with(IntervalValue(IntervalValue::minus_infinity(), rhs.ub()));
				break;
			}
			case CmpStmt::Predicate::FCMP_FALSE: break;
			case CmpStmt::Predicate::FCMP_TRUE: break;
			default: assert(false && "implement this part"); abort();
			}
		}
	}

	as = new_es;
	return true;
}

bool AbstractExecution::isSwitchBranchFeasible(const SVFVar* var, s64_t succ, AbstractState& as) {
	AbstractState new_es = as;
	IntervalValue& switch_cond = new_es[var->getId()].getInterval();
	s64_t value = succ;
	FIFOWorkList<const SVFStmt*> workList;
	for (SVFStmt* cmpVarInStmt : var->getInEdges()) {
		workList.push(cmpVarInStmt);
	}
	switch_cond.meet_with(IntervalValue(value, value));
	if (switch_cond.isBottom()) {
		return false;
	}
	while (!workList.empty()) {
		const SVFStmt* stmt = workList.pop();
		if (SVFUtil::isa<CopyStmt>(stmt)) {
			IntervalValue& copy_cond = new_es[var->getId()].getInterval();
			copy_cond.meet_with(IntervalValue(value, value));
		}
		else if (const LoadStmt* load = SVFUtil::dyn_cast<LoadStmt>(stmt)) {
			if (new_es.inVarToAddrsTable(load->getRHSVarID())) {
				AddressValue& addrs = new_es[load->getRHSVarID()].getAddrs();
				for (const auto& addr : addrs) {
					NodeID objId = SVF::AbstractState::getInternalID(addr);
					if (new_es.inAddrToValTable(objId)) {
						new_es.load(addr).meet_with(switch_cond);
					}
				}
			}
		}
	}
	as = new_es;
	return true;
}

bool AbstractExecution::isBranchFeasible(const IntraCFGEdge* intraEdge, AbstractState& as) {
	const SVFValue* cond = intraEdge->getCondition();
	NodeID cmpID = _svfir->getValueNode(cond);
	SVFVar* cmpVar = _svfir->getGNode(cmpID);
	if (cmpVar->getInEdges().empty()) {
		return isSwitchBranchFeasible(cmpVar, intraEdge->getSuccessorCondValue(), as);
	}
	else {
		assert(!cmpVar->getInEdges().empty() && "no in edges?");
		SVFStmt* cmpVarInStmt = *cmpVar->getInEdges().begin();
		if (const CmpStmt* cmpStmt = SVFUtil::dyn_cast<CmpStmt>(cmpVarInStmt)) {
			return isCmpBranchFeasible(cmpStmt, intraEdge->getSuccessorCondValue(), as);
		}
		else {
			return isSwitchBranchFeasible(cmpVar, intraEdge->getSuccessorCondValue(), as);
		}
	}
}

/// handle global node
void AbstractExecution::handleGlobalNode() {
	AbstractState as;
	const ICFGNode* node = _icfg->getGlobalICFGNode();
	_postAbsTrace[node] = _preAbsTrace[node];
	_postAbsTrace[node][0] = AddressValue();
	// Global Node, we just need to handle addr, load, store, copy and gep
	for (const SVFStmt* stmt : node->getSVFStmts()) {
		updateAbsState(stmt);
	}
}

void AbstractExecution::ensureAllAssertsValidated() {
	for (auto it = _svfir->getICFG()->begin(); it != _svfir->getICFG()->end(); ++it) {
		const ICFGNode* node = it->second;
		if (const CallICFGNode* call = SVFUtil::dyn_cast<CallICFGNode>(node)) {
			if (const SVFFunction* fun = SVFUtil::getCallee(call->getCallSite())) {
				if (fun->getName() == "svf_assert" || fun->getName() == "OVERFLOW") {
					if (_assert_points.find(call) == _assert_points.end()) {
						std::stringstream ss;
						ss << "The stub function calliste (svf_assert or OVERFLOW) has not been checked: "
						   << call->getCallSite()->toString();
						std::cerr << ss.str() << std::endl;
						assert(false);
					}
				}
			}
		}
	}
}