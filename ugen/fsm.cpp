#include "eval.h"
#include "ugen_util.h"
#include <FFT_UGens.h>
#include <iostream>
#include <string>

using namespace std;

InterfaceTable* ft;
static Evaluator eval;

struct FSM : public Unit
{
    string code;
    PyObject* obj;
};

extern "C" {
void FSM_Ctor(FSM* unit);
void FSM_Dtor(FSM* unit);
void FSM_Next(FSM* unit, int numSamples);
void FSM_NextVoid(FSM* unit, int numSamples);
}

void
done(FSM* unit)
{
    unit->mDone = true;
    DoneAction(13, unit);
    SETCALC(FSM_NextVoid);
}

bool
checkError(FSM* unit)
{
    if (eval.checkError()) {
        eval.printError();
        done(unit);
        return true;
    }
    return false;
}

Type
parseType(string& type)
{
    if (type == "Integer" || type == "Float")
        return Type::Float;
    if (type == "Buffer")
        return Type::FloatBuffer;
    if (type == "FFT")
        return Type::ComplexBuffer;
    return Type::Unsupported;
}

void
FSM_Ctor(FSM* unit)
{
    cout << "FSM_Ctor" << endl;
    new (unit) FSM;

    int idx = 0;
    unit->code = readString(unit, idx);
    unit->obj = eval.compile(unit->code);
    if (checkError(unit))
        return;

    int numArgs = readAtom<int>(unit, idx);
    for (int i = 0; i < numArgs; i++) {
        string name = readString(unit, idx);
        string typeStr = readString(unit, idx);
        Type type = parseType(typeStr);
        switch (type) {
            case Type::Float: {
                float val = readAtom<float>(unit, idx);
                eval.defineVariable(name, Object(val));
                break;
            }
            case Type::FloatBuffer: {
                uint32 bufNum = readAtom<uint32>(unit, idx);
                FloatBuffer buf = getFloatBuffer(unit, bufNum);
                eval.defineVariable(name, Object(buf));
                break;
            }
            case Type::ComplexBuffer: {
                uint32 bufNum = readAtom<uint32>(unit, idx);
                ComplexBuffer buf = getComplexBuffer(unit, bufNum);
                eval.defineVariable(name, Object(buf));
                break;
            }
            case Type::Unsupported:
                cout << "Argument '" << name << "' has unsupported type '"
                     << typeStr << "'" << endl;
                done(unit);
                return;
        }
        cout << name << " :: " << typeStr << endl;
    }

    SETCALC(FSM_Next);
}

void
FSM_Dtor(FSM* unit)
{
    cout << "FSM_Dtor" << endl;
    unit->~FSM();
}

void
FSM_Next(FSM* unit, int)
{
    eval.eval(unit->obj);
    if (checkError(unit))
        return;
}

void
FSM_NextVoid(FSM*, int)
{
}

PluginLoad(FSM)
{
    ft = inTable;
    DefineDtorUnit(FSM);
}
