#include "protohelpers.h"

#include "pushinput.h"

#include <util/stream/walk.h>
#include <util/stream/zlib.h>

#include <contrib/libs/protobuf/messagext.h>


static Stroka SplitNewLines(const Stroka& msg, TOutputStream& out) {
    // new line means we want to start our error message from new line
    if (msg.has_prefix("\n")) {
        TStringBuf fixed = msg;
        while (fixed.has_prefix("\n"))
            fixed = fixed.SubStr(1);
        out << "\n";
        return ::ToString(fixed);
    } else
        return msg;
}

void TErrorCollector::AddError(const TProtoStringType& filename, int line, int column, const TProtoStringType& message)
{
    Stroka fixedMsg = SplitNewLines(message, Cerr);
    Cerr << filename << ":";
    if (line < 0)
        Cerr << " ";
    AddError(line, column, fixedMsg);
}

void TErrorCollector::AddError(int line, int column, const TProtoStringType& message)
{
    Stroka fixedMsg = SplitNewLines(message, Cerr);

    // TODO: calculate real (original) file position
    if (line >= 0)
        Cerr << line + 1 << ":" << column + 1 << ":\t";
    Cerr << fixedMsg << Endl;
}

void TErrorCollector::AddError(const TProtoStringType& filename, const TProtoStringType& element_name,
                               const NProtoBuf::Message* /*descriptor*/,
                               NProtoBuf::DescriptorPool::ErrorCollector::ErrorLocation /*location*/,
                               const TProtoStringType& message)
{
    Stroka fixedMsg = SplitNewLines(message, Cerr);
    Cerr << filename << ": " << element_name << ": " << fixedMsg << Endl;
}

void TErrorCollector::AddErrorAtCurrentFile(int line, int column, const TProtoStringType& message)
{
    if (CurrentFile.empty())
        AddError(line, column, message);
    else
        AddError(CurrentFile, line, column, message);
}

void TErrorCollector::AddError(const TDiskSourceTree* tree, int line, int column, const TProtoStringType& message) {
    if (tree != NULL) {
        Stroka file = tree->CurrentDiskFile();
        if (!file.empty()) {
            Stroka virtualFile;
            if (tree->DiskFileToVirtualFile(file, virtualFile))
                file = virtualFile;
            AddError(file, line, column, message);
            return;
        }
    }
    // cannot resolve filename, print what we have
    AddError(line, column, message);
}



class TDiskSourceTree::TPathStack {
public:
    struct TPath: public TIntrusiveSListItem<TPath> {
        Stroka DiskFile;        // file path (full non-virtual)
        TProtobufDiskSourceTree Tree;

        TPath(const Stroka& file = Stroka())
            : DiskFile(file)
        {
            if (!DiskFile.empty()) {
                // we should first call correctpath(), otherwise GetDirName fails...
                Stroka corrected = DiskFile;
                correctpath(corrected);
                AddPathAsRoot(GetDirName(corrected), Tree);
            }
        }
    };

    ~TPathStack() {
        Clear();
    }

    void Clear() {
        while (!Stack.Empty())
            delete Stack.PopFront();
    }

    TPath& Top() {
        return Stack.Empty() ? EmptyPath : *Stack.Begin();
    }

    const TPath& Top() const {
        return Stack.Empty() ? EmptyPath : *Stack.Begin();
    }

    bool Empty() const {
        return Stack.Empty();
    }

    void Push(const Stroka& path) {
        THolder<TPath> p(new TPath(path));
        Stack.PushFront(p.Release());
    }

    Stroka Pop() {
        if (!Stack.Empty()) {
            THolder<TPath> p(Stack.PopFront());
            return p->DiskFile;
        } else
            return Stroka();
    }

private:
    TIntrusiveSList<TPath> Stack;
    TPath EmptyPath;
};

TDiskSourceTree::TFileBase::~TFileBase() {
    Stack.Pop();
}

TDiskSourceTree::TDiskSourceTree()
    : ImportTree(new TProtobufDiskSourceTree)
    , RootPath(new TPathStack)
    , CurPath(new TPathStack)
{
}

TDiskSourceTree::TDiskSourceTree(const yvector<Stroka>& importPaths)
    : ImportTree(new TProtobufDiskSourceTree)
    , RootPath(new TPathStack)
    , CurPath(new TPathStack)
{
    AddImportPaths(importPaths);
}

TDiskSourceTree::~TDiskSourceTree() {
}

inline const TDiskSourceTree::TPathStack* TDiskSourceTree::CurrentPathStack() const {
    return CurPath->Empty() ? RootPath.Get() : CurPath.Get();
}

Stroka TDiskSourceTree::CurrentDiskFile() const {
    return CurrentPathStack()->Top().DiskFile;
}

Stroka TDiskSourceTree::RootDiskFile() const {
    return RootPath->Top().DiskFile;
}

Stroka TDiskSourceTree::PushCurrentFile(const Stroka& file) {
    Stroka diskFile = RealPath(file);
    CurPath->Push(diskFile);
    return FindVirtualFile(diskFile);
}

Stroka TDiskSourceTree::PushRootFile(const Stroka& file) {
    Stroka diskFile = RealPath(file);
    RootPath->Push(diskFile);
    CurPath->Clear();             // reset current dirs
    return FindVirtualFile(diskFile);
}

void TDiskSourceTree::AddImportPaths(const Stroka& path) {
    yvector<Stroka> paths;
    Split(path, ":", paths);
    for (size_t i = 0; i != paths.size(); ++i) {
        Stroka corr = RealPath(paths[i]);
        correctpath(corr);
        AddPathAsRoot(corr, *ImportTree);
    }
}

Stroka TDiskSourceTree::FindVirtualFile(const Stroka& diskFile) const {
    // special case: empty string maps to empty string
    if (!diskFile)
        return Stroka();

    Stroka virtualFile, shadowingDiskFile;
    EResolveRes res = DiskFileToVirtualFile(diskFile, &virtualFile, &shadowingDiskFile);
    if (res != TProtobufDiskSourceTree::SUCCESS)
        RaiseMappingError(res, diskFile, shadowingDiskFile);
    return virtualFile;
}

bool TDiskSourceTree::DiskFileToVirtualFile(const Stroka& diskFile, Stroka& virtualFile) const {
    Stroka virt, shad;
    if (DiskFileToVirtualFile(diskFile, &virt, &shad) == TProtobufDiskSourceTree::SUCCESS) {
        virtualFile = virt;
        return true;
    } else
        return false;
}

static inline bool IsSlash(const char ch) {
    return ch == '/' || ch == '\\';
}

// true if @path starts with "./" and removes this prefix
static inline bool StartsWithExplicitCurDir(const TStringBuf& path) {
    return path.has_prefix(".") && (path.size() <= 1 || IsSlash(path[1]));
}

static inline bool ContainsParentReference(const TStringBuf& path) {
    size_t pos = path.find("..");
    if (pos == TStringBuf::npos)
        return false;

    return (pos <= 0 || IsSlash(path[pos - 1])) &&
           (pos >= path.size() - 2 || IsSlash(path[pos + 2]));
}

bool TDiskSourceTree::VirtualFileToDiskFile(const Stroka& virtualFile, Stroka& diskFile) const {
    // lock is acquired due to non-const of VirtualFileToDiskFile(), so it may be thread unsafe (or may be not?)
    TGuard<TMutex> lock(Mutex);

    // if @virtualFile starts with an explicit "./" or contains parent folder refs ".."
    // search only in current directory
    if (StartsWithExplicitCurDir(virtualFile) || ContainsParentReference(virtualFile)) {
        Stroka resolved = virtualFile;
        resolvepath(resolved, CurrentDir());
        correctpath(resolved);
        if (!isexist(~resolved))
            return false;
        diskFile = resolved;
        return true;
    }

    // an order is important
    return CurPath->Top().Tree.VirtualFileToDiskFile(virtualFile, &diskFile)
        || RootPath->Top().Tree.VirtualFileToDiskFile(virtualFile, &diskFile)
        || ImportTree->VirtualFileToDiskFile(virtualFile, &diskFile);
}

Stroka TDiskSourceTree::CanonicName(const Stroka& virtualFile) const {
    Stroka diskFile;
    if (VirtualFileToDiskFile(virtualFile, diskFile)) {
        Stroka remappedFile;
        if (DiskFileToVirtualFile(diskFile, remappedFile))
            return remappedFile;
    }
    return virtualFile;
}

static inline TAutoPtr<TInputStream> OpenMaybeCompressedFile(const Stroka& diskFile) {
    THolder<TMappedFileInput> input(new TMappedFileInput(diskFile));
    if (diskFile.has_suffix(".gz"))
        return new TStreamAdaptor<IZeroCopyInput, TZLibDecompress>(input.Release());
    else
        return input.Release();
}


TAutoPtr<TInputStream> TDiskSourceTree::OpenDiskFile(const Stroka& diskFile) const {
    if (!isexist(~diskFile))
        return NULL;
    TAutoPtr<TInputStream> input = OpenMaybeCompressedFile(diskFile);
    return MakeInputAdaptor<TSkipBOMInput>(input.Release());
}

TAutoPtr<TInputStream> TDiskSourceTree::OpenVirtualFile(const Stroka& virtualFile) const {
    Stroka diskFile;
    return VirtualFileToDiskFile(virtualFile, diskFile) ? OpenDiskFile(diskFile) : NULL;
}

NProtoBuf::io::ZeroCopyInputStream* TDiskSourceTree::Open(const Stroka& virtualFile) {
    TAutoPtr<TInputStream> input = OpenVirtualFile(virtualFile);
    if (!input)
        return NULL;

    TMemoryInput* mem = dynamic_cast<TMemoryInput*>(input.Get());
    if (mem)
        return new TStreamAdaptor<TMemoryInput, NGzt::TMemoryInputStreamAdaptor>(static_cast<TMemoryInput*>(input.Release()));
    else
        return new TStreamAdaptor<TInputStream, NProtoBuf::io::TCopyingInputStreamAdaptor>(input.Release());
}


TDiskSourceTree::EResolveRes TDiskSourceTree::DiskFileToVirtualFile(const Stroka& diskFile, Stroka* virtualFile,
                                                                    Stroka* shadowingDiskFile) const {
    // lock is acquired due to non-const of DiskFileToVirtualFile(), so it may be thread unsafe (or may be not?)
    TGuard<TMutex> lock(Mutex);
    // Try returning virtual path from root or import paths in the first place, than go searching from current dir
    EResolveRes res = RootPath->Top().Tree.DiskFileToVirtualFile(diskFile, virtualFile, shadowingDiskFile);
    if (res != TProtobufDiskSourceTree::SUCCESS) {
        res = ImportTree->DiskFileToVirtualFile(diskFile, virtualFile, shadowingDiskFile);
        if (res != TProtobufDiskSourceTree::SUCCESS)
            res = CurPath->Top().Tree.DiskFileToVirtualFile(diskFile, virtualFile, shadowingDiskFile);
    }
    return res;
}

void TDiskSourceTree::RaiseMappingError(EResolveRes res, const Stroka& file, const Stroka& shadowingDiskFile) {
    YASSERT(res != TProtobufDiskSourceTree::SUCCESS);
    Cerr << "Virtual file mapping error.\n"
         << "Requested file: " << file << "\n";
    if (res == TProtobufDiskSourceTree::SHADOWED)
        Cerr << "Shadowing file: " << shadowingDiskFile;
    else if (res == TProtobufDiskSourceTree::CANNOT_OPEN)
        Cerr << "Cannot open file.";
    else if (res == TProtobufDiskSourceTree::NO_MAPPING)
        Cerr << "No mapping.";
    else
        Cerr << "Unknown error.";
    Cerr << Endl;
    ythrow yexception() << "Virtual file mapping error.";
}


bool MessageLess::operator()(const NProtoBuf::Message* a, const NProtoBuf::Message* b)
{
    const NProtoBuf::Descriptor* da = a->GetDescriptor();
    const NProtoBuf::Descriptor* db = b->GetDescriptor();
    YASSERT(da == db);
    a->SerializeToString(&BufferA);
    b->SerializeToString(&BufferB);
    return BufferA < BufferB;
}

TJsonPrinter::TJsonPrinter() {
    Config.EnumMode = NProtobufJson::TProto2JsonConfig::EnumName;
    //Config.StringTransforms.push_back(new NProtobufJson::TSafeUtf8CEscapeTransform());
}

void TJsonPrinter::ToString(const NProtoBuf::Message& message, Stroka& out)
{
    out.clear();
    NProtobufJson::Proto2Json(message, out, Config);
}

Stroka TJsonPrinter::ToString(const NProtoBuf::Message& message)
{
    Stroka res;
    NProtobufJson::Proto2Json(message, res, Config);
    return res;
}

void TJsonPrinter::ToStream(const NProtoBuf::Message& message, TOutputStream& out)
{
    NProtobufJson::Proto2Json(message, out, Config);
}


bool ReserializeProto(const NProtoBuf::Message* from, NProtoBuf::Message* to) {

    // for small messages (up to 1Kb) stack buffer is used.
    const size_t static_buffer_size = 1024;
    char static_buffer[static_buffer_size];
    TArrayHolder<char> dynamic_buffer;

    char* data = NULL;
    size_t data_size = from->ByteSize();    //the size of @from is cached now
    if (data_size > static_buffer_size) {
        dynamic_buffer.Reset(new char[data_size]);
        data = dynamic_buffer.Get();
    } else
        data = static_buffer;

    NProtoBuf::io::ArrayOutputStream output(data, data_size);
    NProtoBuf::io::CodedOutputStream encoder(&output);
    from->SerializeWithCachedSizes(&encoder);

    NProtoBuf::io::CodedInputStream decoder((ui8*)data, data_size);
    return to->ParseFromCodedStream(&decoder);
}
