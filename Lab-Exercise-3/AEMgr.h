//===- AEMgr.h -- Abstract Execution Manual Translations---------------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
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

//
// Created on 2024/1/10.
//

#include "AE/Core/ICFGWTO.h"
#include "AE/Core/AbstractState.h"
#include "Util/SVFBugReport.h"
#include "WPA/Andersen.h"

namespace SVF {
	class IntervalExeState;
	class IntervalValue;
	class ExeState;
	class SVFIR2ItvExeState;

	class AEState: public AbstractState {
	 public:
		AbstractValue loadValue(NodeID varId) {
			AbstractValue res;
			for (auto addr : (*this)[varId].getAddrs()) {
				res.join_with(load(addr)); // q = *p
			}
			return res;
		}

		void storeValue(NodeID varId, AbstractValue val) {
			for (auto addr : (*this)[varId].getAddrs()) {
				store(addr, val); // *p = q
			}
		}

		AEState widening(const AEState& other) {
			AbstractState widened = AbstractState::widening(other);
			return AEState(static_cast<const AEState&>(widened));
		}

		AEState narrowing(const AEState& other) {
			AbstractState narrowed = AbstractState::narrowing(other);
			return AEState(static_cast<const AEState&>(narrowed));
		}

	};

	class AbstractExecutionMgr {
	 public:
		AbstractExecutionMgr() = default;
		~AbstractExecutionMgr() = default;
		void test1();
		void test2();
		void test3();
		void test4();
		void test5();
		void test6();
		void test7();
		void test8();

		void reset() {
			_strToID.clear();
			as.clear();
		};

		NodeID getInternalID(NodeID addr) {
			return AddressValue::getInternalID(addr);
		}

		NodeID getNodeID(std::string name) {
			auto it = _strToID.find(name);
			if (it != _strToID.end())
				return it->second;
			else {
				_strToID[name] = ++currentExprIdx;
				return currentExprIdx;
			}
		}

		void removeNodeID(std::string name) {
			auto it = _strToID.find(name);
			if (it != _strToID.end())
				_strToID.erase(it);
			else
				assert(false && "removeNodeID: name not found");
		}

		NodeID getNodeID(std::string name, u32_t size) {
			auto it = _strToID.find(name);
			if (it != _strToID.end())
				return it->second;
			else {
				_strToID[name] = ++currentExprIdx;
				currentExprIdx += (size - 1);
				return _strToID[name];
			}
		}

		u32_t getVirtualAddr(NodeID id) const {
			return AddressMask + id;
		}

		u32_t getMemObjAddress(std::string name) {
			auto it = _strToID.find(name);
			if (it == _strToID.end()) {
				assert(false && "");
				abort();
			}
			else {
				u32_t res = _strToID[name];
				return AddressMask + res;
			}
		}

		NodeID getGepObjAddress(std::string arr_name, u32_t offset) {
			auto iter = _strToID.find(arr_name);
			assert(iter != _strToID.end() && "Gep BaseObject expr not found?");
			u32_t baseObjID = iter->second;
			u32_t gepObj = baseObjID + offset;
			if (baseObjID == gepObj) {
				return AddressMask + baseObjID;
			}
			else {
				return AddressMask + gepObj;
			}
		}

		bool svf_assert(AbstractValue absv) {
			IntervalValue iv = absv.getInterval();
			if (iv.is_numeral()) {
				if (iv.getNumeral() == 0) {
					SVFUtil::outs() << SVFUtil::errMsg("\t FAILURE :") << "the assertion is unsatisfiable\n";
					assert(false);
				}
				else {
					SVFUtil::outs() << SVFUtil::sucMsg("\t SUCCESS :") << "the assertion is successfully verified\n";
					return true;
				}
			}
			else {
				SVFUtil::outs() << SVFUtil::errMsg("\t FAILURE :") << "the assertion is unsatisfiable\n";
				assert(false);
			}
		}
		static u32_t currentExprIdx;

	 private:
		AEState as;
		Map<std::string, NodeID> _strToID;
	};
} // namespace SVF