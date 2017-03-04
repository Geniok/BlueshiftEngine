#include "Precompiled.h"
#include "Render/Render.h"
#include "Core/Cmds.h"

BE_NAMESPACE_BEGIN

Skeleton *          SkeletonManager::defaultSkeleton;
SkeletonManager     skeletonManager;

void SkeletonManager::Init() {
    cmdSystem.AddCommand(L"listSkeletons", Cmd_ListSkeletons);
    cmdSystem.AddCommand(L"reloadSkeleton", Cmd_ReloadSkeleton);

    skeletonHashMap.Init(1024, 64, 64);

    // create default skeleton
    defaultSkeleton = AllocSkeleton("_defaultSkeleton");
    defaultSkeleton->permanence = true;
    defaultSkeleton->CreateDefaultSkeleton();
}

void SkeletonManager::Shutdown() {
    cmdSystem.RemoveCommand(L"listSkeletons");
    cmdSystem.RemoveCommand(L"reloadSkeleton");

    skeletonHashMap.DeleteContents(true);
}

Skeleton *SkeletonManager::AllocSkeleton(const char *hashName) {
    if (skeletonHashMap.Get(hashName)) {
        BE_FATALERROR(L"%hs skeleton already allocated", hashName);
    }

    Skeleton *skeleton = new Skeleton;
    skeleton->hashName = hashName;
    skeleton->name = hashName;
    skeleton->name.StripPath();
    skeleton->name.StripFileExtension();
    skeleton->refCount = 1;
    skeletonHashMap.Set(skeleton->hashName, skeleton);

    return skeleton;
}

void SkeletonManager::DestroySkeleton(Skeleton *skeleton) {
    if (skeleton->refCount > 1) {
        BE_WARNLOG(L"SkeletonManager::DestroySkeleton: skeleton '%hs' has %i reference count\n", skeleton->name.c_str(), skeleton->refCount);
    }

    skeletonHashMap.Remove(skeleton->hashName);
    delete skeleton;
}

void SkeletonManager::ReleaseSkeleton(Skeleton *skeleton, bool immediateDestroy) {
    if (skeleton->permanence) {
        return;
    }

    // 레퍼런스 카운터가 0 인 skeleton 만 제거한다
    if (skeleton->refCount > 0) {
        skeleton->refCount--;
    }

    if (immediateDestroy && skeleton->refCount == 0) {
        DestroySkeleton(skeleton);
    }
}

void SkeletonManager::DestroyUnusedSkeletons() {
    Array<Skeleton *> removeArray;

    for (int i = 0; i < skeletonHashMap.Count(); i++) {
        const auto *entry = skeletonHashMap.GetByIndex(i);
        Skeleton *skeleton = entry->second;

        if (skeleton && !skeleton->permanence && skeleton->refCount == 0) {
            removeArray.Append(skeleton);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroySkeleton(removeArray[i]);
    }
}

Skeleton *SkeletonManager::FindSkeleton(const char *hashName) const {
    const auto *entry = skeletonHashMap.Get(Str(hashName));
    if (entry) {
        return entry->second;
    }

    return nullptr;
}

Skeleton *SkeletonManager::GetSkeleton(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return defaultSkeleton;
    }

    Skeleton *skeleton = FindSkeleton(hashName);
    if (skeleton) {
        skeleton->refCount++;
        return skeleton;
    }

    skeleton = AllocSkeleton(hashName);
    if (!skeleton->Load(hashName)) {
        DestroySkeleton(skeleton);
        return defaultSkeleton;
    }

    return skeleton;
}

//--------------------------------------------------------------------------------------------------

void SkeletonManager::Cmd_ListSkeletons(const CmdArgs &args) {
    int count = 0;

    for (int i = 0; i < skeletonManager.skeletonHashMap.Count(); i++) {
        const auto *entry = skeletonManager.skeletonHashMap.GetByIndex(i);
        Skeleton *skeleton = entry->second;

        BE_LOG(L"%3d refs %3d joints : %hs\n",
            skeleton->refCount,
            skeleton->numJoints,
            skeleton->hashName.c_str());

        count++;
    }

    BE_LOG(L"%i total skeletones\n", count);
}

void SkeletonManager::Cmd_ReloadSkeleton(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG(L"reloadSkeleton <filename>\n");
        return;
    }

    if (!WStr::Icmp(args.Argv(1), L"all")) {
        int count = skeletonManager.skeletonHashMap.Count();

        for (int i = 0; i < count; i++) {
            const auto *entry = skeletonManager.skeletonHashMap.GetByIndex(i);
            Skeleton *skeleton = entry->second;
            skeleton->Reload();
        }
    } else {
        Skeleton *skeleton = skeletonManager.FindSkeleton(WStr::ToStr(args.Argv(1)));
        if (!skeleton) {
            BE_WARNLOG(L"Couldn't find skeleton to reload \"%ls\"\n", args.Argv(1));
            return;
        }

        skeleton->Reload();
    }
}

BE_NAMESPACE_END
