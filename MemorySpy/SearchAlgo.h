/*

SearchAlgo - AhoCorasick implementation (based on NFA to DFA conversion)
(c) B. Blechschmidt (Ovex), 2015
http://Blechschmidt.Saarland

Memory deallocation not fully implemented. Nevertheless, this code is sufficient for demonstration purposes.

*/


#pragma once
#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC "-funsigned-char"

#ifndef POLYSTRINGSEARCH_H
#define POLYSTRINGSEARCH_H

#include <list>
#include <map>
#include <unordered_map>
#include <iostream>
#include <limits>
#include <fstream>
#include <unordered_set>

using namespace std;

//Make life somewhat easier
#define iter(list1) for (auto it = list1.begin(); it != list1.end(); it++)
#define iter2(list2) for (auto it2 = list2.begin(); it2 != list2.end(); it2++)
#define iter3(list3) for (auto it3 = list3.begin(); it3 != list3.end(); it3++)
#define iter4(list4) for (auto it4 = list4.begin(); it4 != list4.end(); it4++)
#define iterp(list5) for (auto it = list5->begin(); it != list5->end(); it++)

#ifndef NULL
#define NULL 0
#endif

namespace Utilities
{
	namespace Set
	{

	}
};

template <typename Input>
class DFA;

template <typename Type>
class ComparableSet : public unordered_set<Type>
{
public:
	bool operator<(const ComparableSet& rhs) const //compare lexically
	{
		auto rit = rhs.begin();
		iterp(this)
		{
			if (rit == rhs.end())
			{
				return false;
			}
			if (*it < *rit)
			{
				return true;
			}
			else if (*it > *rit)
			{
				return false;
			}
			rit++;
		}
		if (rit == rhs.end())
		{
			return false;
		}
		else
		{
			return true;
		}
	}
};

template <typename Input>
class NFA //Non-deterministic finite automaton
{
public:



	list<pair<Input, NFA<Input>*>> Transitions;
	bool Accepting;

	void Debug()
	{
		unordered_set<NFA*> DebuggedStates;
		DebugHelper(DebuggedStates);
		DebuggedStates.clear();
	}

	unordered_set<NFA<Input>*> GetFollowers(Input Character)
	{
		unordered_set<NFA<Input>*> ResultSet;
		iter(Transitions)
		{
			if (it->first == Character)
			{
				ResultSet.insert(it->second);
			}
		}
		return ResultSet;
	}

	DFA<Input>* LookupDFA(map<ComparableSet<NFA<Input>*>, DFA<Input>*> &LookupFunction, ComparableSet<NFA<Input>*> &Set)
	{
		iter(LookupFunction)
		{
			if (it->first == Set)
			{
				return it->second;
			}
		}
		return NULL;
	}

	DFA<Input>* ToDFA() //http://en.wikipedia.org/wiki/Powerset_construction
	{
		NFA<Input>* NFAI = this;
		unordered_set<Input> InputSymbols = NFAI->GetInputSymbols();

		typedef unordered_set<NFA<Input>*> NFASet;

		map<ComparableSet<NFA<Input>*>, DFA<Input>*> LookupFunction;

		DFA<Input>* q0_ = new DFA<Input>();
		q0_->Accepting = NFAI->Accepting;

		ComparableSet<NFA<Input>*> StartSet;
		StartSet.insert(NFAI);
		LookupFunction.insert(make_pair(StartSet, q0_));

		bool TransitionChanged = false;
		do
		{
			TransitionChanged = false;
			iter(LookupFunction)
			{
				ComparableSet<NFA<Input>*> q_Set = it->first;
				iter2(InputSymbols)
				{
					ComparableSet<NFA<Input>*> ReachableStates;
					iter3(q_Set)
					{
						iter4((*it3)->Transitions)
						{
							if (it4->first == *it2)
							{
								ReachableStates.insert(it4->second);
							}
						}
					}
					if (ReachableStates.size() > 0)
					{
						DFA<Input>* LookedUpDFA = LookupDFA(LookupFunction, ReachableStates);
						if (LookedUpDFA == NULL)
						{
							LookedUpDFA = new DFA<Input>();
							LookupFunction.insert(make_pair(ReachableStates, LookedUpDFA));
							DFA<Input>* LastState = LookupDFA(LookupFunction, q_Set);
							LastState->Transitions.insert(make_pair(*it2, LookedUpDFA));
							TransitionChanged = true;
						}
						else
						{
							DFA<Input>* LastState = LookupDFA(LookupFunction, q_Set);
							LastState->Transitions.insert(make_pair(*it2, LookedUpDFA));
						}
						bool Accepting = true;
						iter3(ReachableStates)
						{
							if (!(*it3)->Accepting)
							{
								Accepting = false;
								break;
							}
						}
						LookedUpDFA->Accepting = Accepting;
					}
				}
			}
		} while (TransitionChanged);

		return q0_;
	}

	unordered_set<Input> GetInputSymbols()
	{
		unordered_set<NFA<Input>*> CheckedStates;
		return GetInputSymbols(CheckedStates);
	}



private:

	unordered_set<Input> MergeSets(unordered_set<Input> &Set1, unordered_set<Input> &Set2)
	{
		unordered_set<Input> ResultSet;
		iter(Set1)
		{
			ResultSet.insert(*it);
		}
		iter(Set2)
		{
			ResultSet.insert(*it);
		}
		return ResultSet;
	}

	unordered_set<Input> GetInputSymbols(unordered_set<NFA<Input>*> &CheckedStates)
	{
		unordered_set<Input> ResultSet;
		if (CheckedStates.find(this) == CheckedStates.end())
		{
			CheckedStates.insert(this);
			iter(Transitions)
			{
				ResultSet.insert(it->first);
				unordered_set<Input> SubSet = it->second->GetInputSymbols(CheckedStates);
				ResultSet = MergeSets(ResultSet, SubSet);
			}
		}
		return ResultSet;
	}

	void DebugHelper(unordered_set<NFA*> &DebuggedStates)
	{
		if (DebuggedStates.find(this) != DebuggedStates.end())
		{
			return;
		}
		DebuggedStates.insert(this);
		cout << this << " (" << Accepting << ") [";

		iter(Transitions)
		{
			cout << "   " << it->first << "->" << it->second;
		}
		cout << "   ]" << endl;
		for (auto it = Transitions.begin(); it != Transitions.end(); it++)
		{
			it->second->DebugHelper(DebuggedStates);
		}
	}
};

template <typename Input>
class DFA //Deterministic finite automaton
{
public:
	map<Input, DFA<Input>*> Transitions;
	bool Accepting;

	NFA<Input>* ToNFA()
	{
		NFA<Input>* NFAI = new NFA<Input>();
		NFAI->Accepting = Accepting;

		iter(Transitions)
		{
			NFAI->Transitions.push_back(make_pair(it->first, it->second->ToNFA()));
		}
	}

	void Debug()
	{
		unordered_set<DFA*> DebuggedStates;
		DebugHelper(DebuggedStates);
		DebuggedStates.clear();
	}
private:

	void DebugHelper(unordered_set<DFA*> &DebuggedStates)
	{
		if (DebuggedStates.find(this) != DebuggedStates.end())
		{
			return;
		}
		DebuggedStates.insert(this);
		cout << this << " (" << Accepting << ") [";

		iter(Transitions)
		{
			cout << "   " << it->first << "->" << it->second;
		}
		cout << "   ]" << endl;

		iter(Transitions)
		{
			it->second->DebugHelper(DebuggedStates);
		}
	}


};

template <typename Input>
class AhoCorasick
{
public:

	struct
	{
	public:
		size_t Size;
		size_t ActualElements;
		int* Associations;
	} Projection;

	struct StateArray
	{
	public:
		StateArray** Transitions;
	};

	NFA<Input>* NFAI = new NFA<Input>();
	DFA<Input>* DFAI;
	unordered_set<Input> Occurences;

	StateArray* Array;
	StateArray* CurrentState;

	AhoCorasick(list<list<Input>> &BinaryStrings)
	{
		Constructor(BinaryStrings);
	}

	AhoCorasick(list<Input*> &Strings)
	{
		list<list < Input>> BinaryStrings = Strings2Binary(Strings);
		Constructor(BinaryStrings);
	}

	list<list<Input>> Strings2Binary(list<Input*> &Strings)
	{
		list<list < Input>> Deque;

		iter(Strings)
		{
			list<Input> WordDeque;
			for (Input* c = *it; *c != 0; c++)
			{
				WordDeque.push_back(*c);
			}
			Deque.push_back(WordDeque);
		}
		return Deque;
	}

	bool GoEdge(Input Character)
	{
		if (Projection.Associations[Character] == -1)
		{
			CurrentState = Array;
		}
		else
		{
			if (CurrentState == NULL)
			{
				CurrentState = Array;
			}
			CurrentState = CurrentState->Transitions[Projection.Associations[Character]];
		}
		return CurrentState == NULL;
	}

private:

	void ApplyAgenda(list<pair<NFA<Input>*, pair<Input, NFA<Input>*>>> &Agenda)
	{
		iter(Agenda)
		{
			it->first->Transitions.push_back(it->second);
		}
	}

	void OccurencesToProjection()
	{
		Projection.ActualElements = Occurences.size();
		Projection.Associations = new int[Projection.Size];
		for (int i = 0; i < Projection.Size; i++)
		{
			Projection.Associations[i] = -1;
		}
		int i = 0;
		iter(Occurences)
		{
			Projection.Associations[*it] = i++;
		}
	}

	void InitializeProjection()
	{
		Projection.Size = numeric_limits<Input>::max() + 1;
		Projection.Associations = new int[Projection.Size]();
		OccurencesToProjection();
	}

	StateArray* CreateStateArray(DFA<Input>* State, StateArray* Root, map<DFA<Input>*, StateArray*> &Lookup)
	{
		if (Lookup.find(State) != Lookup.end())
		{
			return Lookup.find(State)->second;
		}
		if (State->Accepting)
		{
			return NULL;
		}
		else
		{
			StateArray* ArrayState = new StateArray();
			Lookup.insert(make_pair(State, ArrayState));
			ArrayState->Transitions = new StateArray*[Projection.ActualElements]();
			for (size_t i = 0; i < Projection.ActualElements; i++)
			{
				if (Root == NULL)
				{
					Root = ArrayState;
				}
				ArrayState->Transitions[i] = Root;
			}
			iter(State->Transitions)
			{
				ArrayState->Transitions[Projection.Associations[it->first]] = CreateStateArray(it->second, Root, Lookup);
			}
			return ArrayState;
		}
	}

	void Constructor(list<list<Input>> &BinaryStrings)
	{
		NFAI->Accepting = false;
		iter(BinaryStrings)
		{
			NFA<Input>* A = NFAFromWord(*it);
			list<pair<NFA<Input>*, pair<Input, NFA<Input>*>>> Agenda;
			MergeSub(NFAI, A, Agenda);
			ApplyAgenda(Agenda);
		}
		Occurences = NFAI->GetInputSymbols();
		DFAI = NFAI->ToDFA();

		InitializeProjection();
		map<DFA<Input>*, StateArray*> Lookup;
		Array = CreateStateArray(DFAI, NULL, Lookup);
		CurrentState = Array;
	}

	void MergeRightSub(NFA<Input> *NFA1, NFA<Input> *NFA2, list<pair<NFA<Input>*, pair<Input, NFA<Input>*>>> &Agenda, unordered_set<NFA<Input>*> &NoCheck)
	{
		if (NoCheck.find(NFA1) != NoCheck.end()) return;
		NoCheck.insert(NFA1);
		for (auto it = NFA1->Transitions.begin(); it != NFA1->Transitions.end(); it++)
		{
			MergeRightSub(it->second, NFA2, Agenda, NoCheck);
		}
		for (auto it = NFA2->Transitions.begin(); it != NFA2->Transitions.end(); it++)
		{
			if (!NFA1->Accepting)
			{
				Agenda.push_back(make_pair(NFA1, make_pair(it->first, it->second)));
				
			}
		}
	}

	void MergeSub(NFA<Input> *NFA1, NFA<Input> *NFA2, list<pair<NFA<Input>*, pair<Input, NFA<Input>*>>> &Agenda)
	{
		unordered_set<NFA<Input>*> NoCheck;
		MergeRightSub(NFA1, NFA2, Agenda, NoCheck);
		NoCheck.clear();
		MergeRightSub(NFA2, NFA1, Agenda, NoCheck);
	}

	static DFA<Input>* DFAFromWord(list<Input> &Word)
	{
		if (Word.begin() == Word.end())
		{
			DFA<Input>* Acceptor = new DFA<Input>();
			Acceptor->Accepting = true;
			return Acceptor;
		}
		else
		{
			DFA<Input>* State = new DFA<Input>();
			State->Accepting = false;
			Input Character = *Word.begin();
			Word.pop_front();
			State->Transitions.insert(make_pair(Character, DFAFromWord(Word)));
			return State;
		}
	}

	static NFA<Input>* NFAFromWord(list<Input> &Word)
	{
		if (Word.begin() == Word.end())
		{
			NFA<Input>* Acceptor = new NFA<Input>();
			Acceptor->Accepting = true;
			return Acceptor;
		}
		else
		{
			NFA<Input>* State = new NFA<Input>();
			State->Accepting = false;
			Input Character = *Word.begin();
			Word.pop_front();
			State->Transitions.push_back(make_pair(Character, NFAFromWord(Word)));
			return State;
		}
	}


};

#endif