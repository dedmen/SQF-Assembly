#include "common.hpp"

namespace intercept::assembly {

	asshelper::asshelper(game_state* gs)
	{
		map["missionnamespace"] = sqf::mission_namespace();
		map["uinamespace"] = sqf::ui_namespace();
		map["parsingnamespace"] = sqf::parsing_namespace();
		map["profilenamespace"] = sqf::profile_namespace();

		fmap["sqrt"] = [](ref<game_instruction> leftinstr, ref<game_instruction> rightinstr, game_value *out, int *died) -> bool {
			*died = 1;
			auto left = dynamic_cast<GameInstructionConst*>(leftinstr.get());
			if (left && left->value.data->type() != 0) { return false; }
			*out = game_value(sqrt((float)left->value));
			return true;
		};
		fmap["mod"] = [](ref<game_instruction> leftinstr, ref<game_instruction> rightinstr, game_value *out, int *died) -> bool {
			*died = 2;
			auto left = dynamic_cast<GameInstructionConst*>(leftinstr.get());
			if (left && left->value.data->type() != 0) { return false; }
			auto right = dynamic_cast<GameInstructionConst*>(leftinstr.get());
			if (right && right->value.data->type() != 0) { return false; }
			*out = game_value(fmodf((float)left->value, (float)right->value));
			return true;
		};
	}
	bool asshelper::containsNular(const char* key) const { return map.find(key) != map.end(); }
	bool asshelper::containsFunc(const char* key) const { return fmap.find(key) != fmap.end(); }
	game_value asshelper::get(const char* key) const { return map.at(key); }
	bool asshelper::get(const char* key, ref<game_instruction> left, ref<game_instruction> right, game_value *out, int *died) const { return fmap.at(key)(left, right, out, died); }

	enum insttype
	{
		NA = -1,
		endStatement,
		push,
		callFunction,
		callOperator,
		assignToLocal,
		assignTo,
		callNular,
		getVariable,
		makeArray
	};
	insttype getinsttype(game_state* gs, ref<game_instruction> instr)
	{
		auto typeHash = typeid(*instr.get()).hash_code();
		std::string typeName = typeid(*instr.get()).name();

		switch (typeHash) {
		case GameInstructionNewExpression::typeIDHash:
			return insttype::endStatement;
		case GameInstructionConst::typeIDHash:
			return insttype::push;
		case GameInstructionFunction::typeIDHash: {
			return insttype::callFunction;
		} break;
		case GameInstructionOperator::typeIDHash:
			return insttype::callOperator;
		case GameInstructionAssignment::typeIDHash: {
			GameInstructionAssignment* inst = static_cast<GameInstructionAssignment*>(instr.get());
			if (inst->forceLocal) {
				return insttype::assignToLocal;
			}
			else {
				return insttype::assignTo;
			}
		} break;
		case GameInstructionVariable::typeIDHash: {
			GameInstructionVariable* inst = static_cast<GameInstructionVariable*>(instr.get());
			auto varname = inst->name;
			if (gs->_scriptNulars.has_key(varname.c_str())) {
				return insttype::callNular;
			}
			else {
				return insttype::getVariable;
			}
		} break;
		case GameInstructionArray::typeIDHash:
			return insttype::makeArray;
		default:
			return insttype::NA;
		}
	}

	bool isconst(game_state* gs, asshelper* nh, ref<game_instruction> instr)
	{
		auto type = getinsttype(gs, instr);
		if (type == insttype::push)
		{
			return true;
		}
		else if (type == insttype::callNular)
		{
			auto inst = static_cast<GameInstructionVariable*>(instr.get());
			if (nh->containsNular(inst->name.c_str()))
			{
				return true;
			}
		}
		return false;
	}
	game_value getconst(game_state* gs, asshelper* nh, ref<game_instruction> instr)
	{
		auto type = getinsttype(gs, instr);
		if (type == insttype::push)
		{
			return static_cast<GameInstructionConst*>(instr.get())->value;
		}
		else if (type == insttype::callNular)
		{
			auto inst = static_cast<GameInstructionVariable*>(instr.get());
			return nh->get(inst->name.c_str());
		}
		else
		{
			throw std::exception();
		}
	}
	void optimize(game_state* gs, asshelper* nh, ref<compact_array<ref<game_instruction>>> instructions)
	{
		size_t count = instructions->size();
		int died = 0;
		for (int i = 0; i < count; i++)
		{
			auto instr = instructions->get(i);
			//GameInstructionConst::make(array);
			switch (getinsttype(gs, instr))
			{
				case insttype::makeArray: {
					auto inst = static_cast<GameInstructionArray*>(instr.get());
					size_t arrsize = inst->size;
					//In case makeArray has zero size, just transform to a push instruction
					if (arrsize == 0)
					{
						instructions->data()[i] = GameInstructionConst::make(auto_array<game_value>());
						break;
					}
					bool abortflag = false;
					//Backtrack - Check if non-constant values are existing
					for (int j = i - arrsize - died; j < i - died; j++)
					{
						if (!isconst(gs, nh, instructions->get(j)))
						{
							abortflag = true;
							break;
						}
					}
					//If abortflag was set, abort conversion
					if (abortflag)
					{
						break;
					}
					//Backtrack - Add elements to array
					auto_array<game_value> arr;
					for (int j = i - arrsize - died; j < i - died; j++)
					{
						arr.push_back(getconst(gs, nh, instructions->get(j)));
					}
					died += arrsize;
					instructions->data()[i] = GameInstructionConst::make(std::move(arr));
				} break;
				case insttype::callFunction: {
					auto inst = static_cast<GameInstructionFunction*>(instr.get());
					game_value valueslot;
					int diedslot;
					if (nh->containsFunc(inst->getFuncName().c_str()) &&
						nh->get(inst->getFuncName().c_str(), instructions->get(i - died - 1), instructions->get(i - died - 2), &valueslot, &diedslot))
					{
						died += diedslot;
						instructions->data()[i] = GameInstructionConst::make(valueslot);
					}
				} break;
			}
			if (died > 0)
			{
				instructions->data()[i - died] = instructions->data()[i];
			}
		}
		for (auto i = instructions->begin() + (count - died); i < instructions->end(); i++)
		{
			i->free();
		}
		instructions->_size -= died;
	}
}