#pragma once

#include "Array.h"
#include "NumberSet.h"
#include "Errors.h"
#include "BitVector.h"
#include <stdio.h>
#include "MachineTypes.h"
#include "Ident.h"

enum InterCode
{
	IC_NONE,
	IC_LOAD_TEMPORARY,
	IC_STORE_TEMPORARY,
	IC_BINARY_OPERATOR,
	IC_UNARY_OPERATOR,
	IC_RELATIONAL_OPERATOR,
	IC_CONVERSION_OPERATOR,
	IC_STORE,
	IC_LOAD,
	IC_LEA,
	IC_COPY,
	IC_TYPECAST,
	IC_CONSTANT,
	IC_BRANCH,
	IC_JUMP,
	IC_PUSH_FRAME,
	IC_POP_FRAME,
	IC_CALL,
	IC_JSR,
	IC_RETURN_VALUE,
	IC_RETURN_STRUCT,
	IC_RETURN
};

enum InterType
{
	IT_NONE,
	IT_UNSIGNED,
	IT_SIGNED,
	IT_FLOAT,
	IT_POINTER,
	IT_BOOL
};

enum InterMemory
{
	IM_NONE,
	IM_PARAM,
	IM_LOCAL,
	IM_GLOBAL,
	IM_FRAME,
	IM_PROCEDURE,
	IM_INDIRECT,
	IM_TEMPORARY,
	IM_ABSOLUTE
};

enum InterOperator
{
	IA_NONE,
	IA_ADD,
	IA_SUB,
	IA_MUL,
	IA_DIVU,
	IA_DIVS,
	IA_MODU,
	IA_MODS,
	IA_OR,
	IA_AND,
	IA_XOR,
	IA_NEG,
	IA_ABS,
	IA_FLOOR,
	IA_CEIL,
	IA_NOT,
	IA_SHL,
	IA_SHR,
	IA_SAR,
	IA_CMPEQ,
	IA_CMPNE,
	IA_CMPGES,
	IA_CMPLES,
	IA_CMPGS,
	IA_CMPLS,
	IA_CMPGEU,
	IA_CMPLEU,
	IA_CMPGU,
	IA_CMPLU,
	IA_FLOAT2INT,
	IA_INT2FLOAT
};

class InterInstruction;
class InterCodeBasicBlock;
class InterCodeProcedure;
class InterVariable;

typedef InterInstruction* InterInstructionPtr;
typedef InterCodeBasicBlock* InterCodeBasicBlockPtr;
typedef InterCodeProcedure* InterCodeProcedurePtr;

typedef GrowingArray<InterType>					GrowingTypeArray;
typedef GrowingArray<int>						GrowingIntArray;
typedef GrowingArray<InterInstructionPtr>		GrowingInstructionPtrArray;
typedef GrowingArray<InterCodeBasicBlockPtr>	GrowingInterCodeBasicBlockPtrArray;
typedef GrowingArray<InterInstruction>			GrowingInstructionArray;
typedef GrowingArray<InterCodeProcedurePtr	>	GrowingInterCodeProcedurePtrArray;


typedef GrowingArray<InterVariable>				GrowingVariableArray;

#define INVALID_TEMPORARY	(-1)

class ValueSet
{
protected:
	InterInstructionPtr	* mInstructions;
	int								mNum, mSize;
public:
	ValueSet(void);
	ValueSet(const ValueSet& values);
	~ValueSet(void);

	void FlushAll(void);
	void FlushCallAliases(void);

	void RemoveValue(int index);
	void InsertValue(InterInstruction& ins);

	void UpdateValue(InterInstruction& ins, const GrowingInstructionPtrArray& tvalue, const NumberSet& aliasedLocals);
};

class TempForwardingTable
{
protected:
	struct Assoc
	{
		int	mAssoc, mSucc, mPred;

		Assoc(void) {}
		Assoc(int assoc, int succ, int pred) { this->mAssoc = assoc; this->mSucc = succ; this->mPred = pred; }
		Assoc(const Assoc& assoc) { this->mAssoc = assoc.mAssoc; this->mSucc = assoc.mSucc; this->mPred = assoc.mPred; }
	};

	GrowingArray<Assoc>		mAssoc;

public:
	TempForwardingTable(void) : mAssoc(Assoc(-1, -1, -1))
	{
	}

	TempForwardingTable(const TempForwardingTable & table) : mAssoc(Assoc(-1, -1, -1))
	{
		for (int i = 0; i < table.mAssoc.Size(); i++)
		{
			mAssoc[i].mAssoc = table.mAssoc[i].mAssoc;
			mAssoc[i].mSucc = table.mAssoc[i].mSucc;
			mAssoc[i].mPred = table.mAssoc[i].mPred;
		}
	}

	void SetSize(int size)
	{
		int i;
		mAssoc.SetSize(size);

		for (i = 0; i < size; i++)
			mAssoc[i] = Assoc(i, i, i);
	}

	void Reset(void)
	{
		int i;

		for (i = 0; i < mAssoc.Size(); i++)
			mAssoc[i] = Assoc(i, i, i);
	}

	int operator[](int n)
	{
		return mAssoc[n].mAssoc;
	}

	void Destroy(int n)
	{
		int i, j;

		if (mAssoc[n].mAssoc == n)
		{
			i = mAssoc[n].mSucc;
			while (i != n)
			{
				j = mAssoc[i].mSucc;
				mAssoc[i] = Assoc(i, i, i);
				i = j;
			}
		}
		else
		{
			mAssoc[mAssoc[n].mPred].mSucc = mAssoc[n].mSucc;
			mAssoc[mAssoc[n].mSucc].mPred = mAssoc[n].mPred;
		}

		mAssoc[n] = Assoc(n, n, n);
	}

	void Build(int from, int to)
	{
		int i;

		from = mAssoc[from].mAssoc;
		to = mAssoc[to].mAssoc;

		if (from != to)
		{
			i = mAssoc[from].mSucc;
			while (i != from)
			{
				mAssoc[i].mAssoc = to;
				i = mAssoc[i].mSucc;
			}
			mAssoc[from].mAssoc = to;

			mAssoc[mAssoc[to].mSucc].mPred = mAssoc[from].mPred;
			mAssoc[mAssoc[from].mPred].mSucc = mAssoc[to].mSucc;
			mAssoc[to].mSucc = from;
			mAssoc[from].mPred = to;
		}
	}
};

class InterVariable
{
public:
	bool				mUsed, mAliased, mPlaced, mAssembler;
	int					mIndex, mSize, mOffset, mAddr;
	int					mNumReferences;
	const uint8		*	mData;
	const Ident		*	mIdent;

	struct Reference
	{
		uint16		mAddr;
		bool		mFunction, mLower, mUpper;
		uint16		mIndex, mOffset;
	}	*mReferences;

	InterVariable(void)
		: mUsed(false), mAliased(false), mPlaced(false), mIndex(-1), mSize(0), mOffset(0), mNumReferences(0), mData(nullptr), mIdent(nullptr), mReferences(nullptr), mAssembler(false)
	{
	}
};


typedef GrowingArray<InterVariable::Reference>		GrowingInterVariableReferenceArray;

class InterInstruction
{
public:
	InterCode							mCode;
	InterType							mTType, mSType[3];
	int									mTTemp, mSTemp[3];
	bool								mSFinal[3];
	__int64								mSIntConst[3];
	double								mSFloatConst[3];
	InterMemory							mMemory;
	InterOperator						mOperator;
	int									mOperandSize;
	int									mVarIndex;
	__int64								mIntValue;
	double								mFloatValue;
	Location							mLocation;

	InterInstruction(void);
	void SetCode(const Location & loc, InterCode code);

	void CollectLocalAddressTemps(GrowingIntArray& localTable);
	void MarkAliasedLocalTemps(const GrowingIntArray& localTable, NumberSet& aliasedLocals);

	void FilterTempUsage(NumberSet& requiredVars, NumberSet& providedVars);
	void FilterVarsUsage(const GrowingVariableArray& localVars, NumberSet& requiredTemps, NumberSet& providedTemps);
	bool RemoveUnusedResultInstructions(InterInstruction* pre, NumberSet& requiredTemps, int numStaticTemps);
	bool RemoveUnusedStoreInstructions(const GrowingVariableArray& localVars, InterInstruction* pre, NumberSet& requiredTemps);
	void PerformValueForwarding(GrowingInstructionPtrArray& tvalue, FastNumberSet& tvalid);

	void LocalRenameRegister(GrowingIntArray& renameTable, int& num, int fixed);
	void GlobalRenameRegister(const GrowingIntArray& renameTable, GrowingTypeArray& temporaries);

	void PerformTempForwarding(TempForwardingTable& forwardingTable);

	void BuildCollisionTable(NumberSet& liveTemps, NumberSet* collisionSets);
	void ReduceTemporaries(const GrowingIntArray& renameTable, GrowingTypeArray& temporaries);

	void CollectActiveTemporaries(FastNumberSet& set);
	void ShrinkActiveTemporaries(FastNumberSet& set, GrowingTypeArray& temporaries);
	
	void CollectSimpleLocals(FastNumberSet& complexLocals, FastNumberSet& simpleLocals, GrowingTypeArray& localTypes);
	void SimpleLocalToTemp(int vindex, int temp);

	void Disassemble(FILE* file);
};


class InterCodeException
{
public:
	InterCodeException()
	{
	}
};

class InterCodeStackException : public InterCodeException
{
public:
	InterCodeStackException()
		: InterCodeException()
	{
	}
};

class InterCodeUndefinedException : public InterCodeException
{
public:
	InterCodeUndefinedException()
		: InterCodeException()
	{
	}
};

class InterCodeTypeMismatchException : public InterCodeException
{
public:
	InterCodeTypeMismatchException()
		: InterCodeException()
	{
	}
};

class InterCodeUninitializedException : public InterCodeException
{
public:
	InterCodeUninitializedException()
		: InterCodeException()
	{
	}
};

class InterCodeBasicBlock
{
public:
	int								mIndex, mNumEntries;
	InterCodeBasicBlock* mTrueJump, * mFalseJump;
	GrowingInstructionArray			mInstructions;

	bool								mVisited;

	NumberSet						mLocalRequiredTemps, mLocalProvidedTemps;
	NumberSet						mEntryRequiredTemps, mEntryProvidedTemps;
	NumberSet						mExitRequiredTemps, exitProvidedTemps;

	NumberSet						mLocalRequiredVars, mLocalProvidedVars;
	NumberSet						mEntryRequiredVars, mEntryProvidedVars;
	NumberSet						mExitRequiredVars, mExitProvidedVars;

	InterCodeBasicBlock(void);
	~InterCodeBasicBlock(void);

	void Append(InterInstruction & code);
	void Close(InterCodeBasicBlock* trueJump, InterCodeBasicBlock* falseJump);

	void CollectEntries(void);
	void GenerateTraces(void);

	void LocalToTemp(int vindex, int temp);

	void CollectLocalAddressTemps(GrowingIntArray& localTable);
	void MarkAliasedLocalTemps(const GrowingIntArray& localTable, NumberSet& aliasedLocals);

	void BuildLocalTempSets(int num, int numFixed);
	void BuildGlobalProvidedTempSet(NumberSet fromProvidedTemps);
	bool BuildGlobalRequiredTempSet(NumberSet& fromRequiredTemps);
	bool RemoveUnusedResultInstructions(int numStaticTemps);

	void BuildLocalVariableSets(const GrowingVariableArray& localVars);
	void BuildGlobalProvidedVariableSet(const GrowingVariableArray& localVars, NumberSet fromProvidedVars);
	bool BuildGlobalRequiredVariableSet(const GrowingVariableArray& localVars, NumberSet& fromRequiredVars);
	bool RemoveUnusedStoreInstructions(const GrowingVariableArray& localVars);

	GrowingIntArray			mEntryRenameTable;
	GrowingIntArray			mExitRenameTable;

	void LocalRenameRegister(const GrowingIntArray& renameTable, int& num, int fixed);
	void BuildGlobalRenameRegisterTable(const GrowingIntArray& renameTable, GrowingIntArray& globalRenameTable);
	void GlobalRenameRegister(const GrowingIntArray& renameTable, GrowingTypeArray& temporaries);

	void CheckValueUsage(InterInstruction& ins, const GrowingInstructionPtrArray& tvalue);
	void PerformTempForwarding(TempForwardingTable& forwardingTable);
	void PerformValueForwarding(const GrowingInstructionPtrArray& tvalue, const ValueSet& values, FastNumberSet& tvalid, const NumberSet& aliasedLocals);
	void PerformMachineSpecificValueUsageCheck(const GrowingInstructionPtrArray& tvalue, FastNumberSet& tvalid);

	void BuildCollisionTable(NumberSet* collisionSets);
	void ReduceTemporaries(const GrowingIntArray& renameTable, GrowingTypeArray& temporaries);

	void CollectSimpleLocals(FastNumberSet& complexLocals, FastNumberSet& simpleLocals, GrowingTypeArray & localTypes);
	void SimpleLocalToTemp(int vindex, int temp);

	void CollectActiveTemporaries(FastNumberSet& set);
	void ShrinkActiveTemporaries(FastNumberSet& set, GrowingTypeArray& temporaries);

	void ApplyExceptionStackChanges(GrowingInterCodeBasicBlockPtrArray& exceptionStack);

	void Disassemble(FILE* file, bool dumpSets);

	void CollectVariables(GrowingVariableArray & globalVars, GrowingVariableArray & localVars);
	void MapVariables(GrowingVariableArray& globalVars, GrowingVariableArray& localVars);
	
	void CollectOuterFrame(int level, int& size);

	bool IsLeafProcedure(void);

	void PeepholeOptimization(void);
};

class InterCodeModule;

class InterCodeProcedure
{
protected:
	GrowingIntArray						mRenameTable, mRenameUnionTable, mGlobalRenameTable;
	TempForwardingTable					mTempForwardingTable;
	GrowingInstructionPtrArray			mValueForwardingTable;
	int									numFixedTemporaries;
	NumberSet							mLocalAliasedSet;

	void ResetVisited(void);
public:
	GrowingInterCodeBasicBlockPtrArray	mBlocks;	
	GrowingTypeArray					mTemporaries;
	GrowingIntArray						mTempOffset;
	int									mTempSize, mCommonFrameSize;
	bool								mLeafProcedure, mNativeProcedure;

	InterCodeModule					*	mModule;
	int									mID;

	int									mLocalSize;
	GrowingVariableArray				mLocalVars;

	Location							mLocation;
	const Ident						*	mIdent;

	InterCodeProcedure(InterCodeModule * module, const Location & location, const Ident * ident);
	~InterCodeProcedure(void);

	int AddTemporary(InterType type);

	void Append(InterCodeBasicBlock * block);
	void Close(void);

//	void Set(InterCodeIDMapper* mapper, BitVector localStructure, Scanner scanner, bool debug);

	void MapVariables(void);
	void ReduceTemporaries(void);
	void Disassemble(const char* name, bool dumpSets = false);
protected:
	void BuildTraces(void);
	void BuildDataFlowSets(void);
	void RenameTemporaries(void);
	void TempForwarding(void);
	void RemoveUnusedInstructions(void);

	void DisassembleDebug(const char* name);
};

class InterCodeModule
{
public:
	InterCodeModule(void);
	~InterCodeModule(void);

	GrowingInterCodeProcedurePtrArray	mProcedures;

	GrowingVariableArray				mGlobalVars;

	void UseGlobal(int index);
};