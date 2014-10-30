#include "BinSaver.h"

TClassFactory<TObjectBase> *pSaverClasses;
void StartRegisterSaveload() {
    if ( !pSaverClasses )
        pSaverClasses = new TClassFactory<TObjectBase>;
}
struct SBasicChunkInit {
    ~SBasicChunkInit() { if ( pSaverClasses ) delete pSaverClasses; }
} initSaver;

//////////////////////////////////////////////////////////////////////////
void IBinSaver::StoreObject(TObjectBase *pObject)
{
    if (pObject) {
        YASSERT(pSaverClasses->GetObjectTypeID(pObject) != -1 && "trying to save unregistered object");
    }

    ui64 ptrId = ((char*)pObject) - ((char*)0);
    if (StableOutput) {
        ui32 id = 0;
        if (pObject) {
            if (!PtrIds.Get())
                PtrIds.Reset(new PtrIdHash);
            PtrIdHash::iterator pFound = PtrIds->find(pObject);
            if (pFound != PtrIds->end())
                id = pFound->second;
            else {
                id = PtrIds->ysize() + 1;
                PtrIds->insert(MakePair(pObject, id));
            }
        }
        ptrId = id;
    }

    DataChunk(&ptrId, sizeof(ptrId));
    if (!Objects.Get())
        Objects.Reset(new CObjectsHash);
    if (ptrId != 0 && Objects->find(ptrId) == Objects->end()) {
        ObjectQueue.push_back(pObject);
        (*Objects)[ptrId];
        int typeId = pSaverClasses->GetObjectTypeID(pObject);
        if (typeId == -1) {
            fprintf(stderr, "IBinSaver: trying to save unregistered object\n");
            abort();
        }
        DataChunk(&typeId, sizeof(typeId));
    }
}

TObjectBase* IBinSaver::LoadObject()
{
    ui64 ptrId = 0;
    DataChunk(&ptrId, sizeof(ptrId));
    if (ptrId != 0) {
        if (!Objects.Get())
            Objects.Reset(new CObjectsHash);
        CObjectsHash::iterator pFound = Objects->find(ptrId);
        if (pFound != Objects->end())
            return pFound->second;
        int typeId;
        DataChunk(&typeId, sizeof(typeId));
        TObjectBase *pObj = pSaverClasses->CreateObject(typeId);
        YASSERT(pObj != 0);
        if (pObj == 0) {
            fprintf(stderr, "IBinSaver: trying to load unregistered object\n");
            abort();
        }
        (*Objects)[ptrId] = pObj;
        ObjectQueue.push_back(pObj);
        return pObj;
    }
    return 0;
}

IBinSaver::~IBinSaver() {
    for (size_t i = 0; i < ObjectQueue.size(); ++i) {
        AddPolymorphicBase(1, ObjectQueue[i]);
    }
}
