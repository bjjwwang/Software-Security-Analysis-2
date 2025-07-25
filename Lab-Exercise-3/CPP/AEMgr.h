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

#include "AE/Core/AbstractState.h"
#include "AE/Core/ICFGWTO.h"
#include "Util/SVFBugReport.h"
#include "WPA/Andersen.h"


namespace SVF {
	class IntervalExeState;
	class IntervalValue;
	class ExeState;
	class SVFIR2ItvExeState;
	class AbstractExecutionMgr;

	class AEState : public AbstractState {
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

		void printAbstractState() const;

	};

	class AbstractExecutionMgr {
		friend AEState;
	 public:
		AbstractExecutionMgr() = default;
		~AbstractExecutionMgr() = default;
		AEState test0();
		AEState test1();
		AEState test2();
		AEState test3();
		AEState test4();
		AEState test5();
		AEState test6();
		AEState test7();
		AEState test8();

		void reset() {
			currentExprIdx = 1;
			_strToID.clear();
		};

		NodeID getIDFromAddr(NodeID addr) {
			return (addr & FlippedAddressMask);
		}

		static AbstractExecutionMgr& getInstance() {
			static AbstractExecutionMgr instance;
			return instance;
		}


		NodeID getNodeID(std::string name) {
			auto it = _strToID.find(name);
			if (it != _strToID.end())
				return it->second;
			else {
				_strToID[name] = currentExprIdx;
				_idToStr[currentExprIdx] = name;
				return currentExprIdx++;
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
			if (offset == 0) {
				return AddressMask + baseObjID;
			} 
			else {
				u32_t gepObj = (baseObjID << 8) + offset;
				return AddressMask + gepObj;
			}
		}

		bool svf_assert(AbstractValue absv) {
			IntervalValue iv = absv.getInterval();
			if (iv.is_numeral()) {
				if (iv.getNumeral() == 0) {
					SVFUtil::outs() << SVFUtil::errMsg("\t FAILURE :") << "the assertion is unsatisfiable\n";
					assert(false);
					abort();
				}
				else {
					SVFUtil::outs() << SVFUtil::sucMsg("\t SUCCESS :") << "the assertion is successfully verified\n";
					return true;
				}
			}
			else {
				SVFUtil::outs() << SVFUtil::errMsg("\t FAILURE :") << "the assertion is unsatisfiable\n";
				assert(false);
				abort();
			}
		}

		u32_t currentExprIdx{1};

	 private:
		Map<std::string, NodeID> _strToID;
		Map<NodeID, std::string> _idToStr;
	};

	inline void AEState::printAbstractState() const {
		AbstractExecutionMgr& mgr = AbstractExecutionMgr::getInstance();
		SVFUtil::outs() << "-----------Var and Value-----------\n";
		u32_t fieldWidth = 20;
		SVFUtil::outs().flags(std::ios::left);
		for (const auto &item: _varToAbsVal) {
			std::ostringstream oss;
			oss << "Var" << item.first << " (" << mgr._idToStr[item.first] << ")";
			SVFUtil::outs() << std::left << std::setw(fieldWidth) << oss.str() << ": ";
			if (item.second.isInterval()) {
				SVFUtil::outs() << " Value: " << item.second.getInterval().toString() << "\n";
			} else if (item.second.isAddr()) {
				SVFUtil::outs() << " Value: {";
				u32_t i = 0;
				for (const auto& addr: item.second.getAddrs()) {
					++i;
					if (i < item.second.getAddrs().size()) {
						SVFUtil::outs() << "0x" << std::hex << addr << ", ";
					} else {
						SVFUtil::outs() << "0x" << std::hex << addr;
					}
				}
				SVFUtil::outs() << "}\n";
			} else {
				SVFUtil::outs() << " Value: ⊥\n";
			}
		}

		for (const auto& item: _addrToAbsVal) {
			std::ostringstream ossAddress;
			ossAddress << "0x" << std::hex << AEState::getVirtualMemAddress(item.first);
			SVFUtil::outs() << std::left << std::setw(fieldWidth) << ossAddress.str() << ": ";
			if (item.second.isInterval()) {
				SVFUtil::outs() << " Value: " << item.second.getInterval().toString() << "\n";
			} else if (item.second.isAddr()) {
				SVFUtil::outs() << " Value: {";
				u32_t i = 0;
				for (const auto& addr: item.second.getAddrs()) {
					++i;
					if (i < item.second.getAddrs().size()) {
						SVFUtil::outs() << "0x" << std::hex << addr << ", ";
					} else {
						SVFUtil::outs() << "0x" << std::hex << addr;
					}
				}
				SVFUtil::outs() << "}\n";
			} else {
				SVFUtil::outs() << " Value: ⊥\n";
			}
		}
		SVFUtil::outs() << "-----------------------------------------\n";
	}
} // namespace SVF
