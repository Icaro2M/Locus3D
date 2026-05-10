#pragma once

#include "EditorSceneSnapshot.h"
#include "../controllers/SceneContext.h"
#include "../EditorState.h"

class EditorSceneSnapshotBuilder
{
public:
    static EditorSceneSnapshot capture(
        const SceneContext& sceneContext,
        const EditorState& editorState
    );

    static SceneObject* restore(
        const EditorSceneSnapshot& snapshot,
        SceneContext& sceneContext,
        EditorState& editorState
    );
};