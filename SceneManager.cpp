#include "SceneManager.h"

#include "IScene.h"
#include "ITransition.h"

SceneManager::~SceneManager() = default;

void SceneManager::Register(SceneId id, std::unique_ptr<IScene> scene)
{
    scene->SetSceneManager(this);
    scenes_[id] = std::move(scene);
}

void SceneManager::Boot(SceneId first)
{
    SwapTo(first);
    if (current_ != nullptr) {
        current_->OnEnter();
    }
}

void SceneManager::RequestChange(SceneId next, std::unique_ptr<ITransition> transition)
{
    if (hasPending_) {
        return;
    }

    transition_.reset();
    pendingId_ = next;
    hasPending_ = true;

    transition_ = std::move(transition);
    if (transition_ != nullptr) {
        transition_->Start();
    }
}

void SceneManager::SwapTo(SceneId id)
{
    if (current_ != nullptr) {
        current_->OnExit();
    }

    currentId_ = id;
    auto it = scenes_.find(id);
    if (it == scenes_.end()) {
        current_ = nullptr;
        return;
    }

    current_ = it->second.get();

    if (!created_[id]) {
        current_->OnCreate();
        created_[id] = true;
    }
}

void SceneManager::Update(float dt)
{
    if (current_ != nullptr) {
        current_->Update(dt);
    }

    if (transition_ != nullptr) {
        transition_->Update(dt);

        if (hasPending_ && transition_->ShouldSwapSceneNow()) {
            SwapTo(pendingId_);
            if (current_ != nullptr) {
                current_->OnEnter();
            }
            hasPending_ = false;
        }

        if (transition_->IsFinished()) {
            transition_.reset();
        }
    } else if (hasPending_) {
        SwapTo(pendingId_);
        if (current_ != nullptr) {
            current_->OnEnter();
        }
        hasPending_ = false;
    }
}

void SceneManager::Draw()
{
    if (current_ != nullptr) {
        current_->Draw();
    }

    if (transition_ != nullptr) {
        transition_->DrawOverlay();
    }
}
