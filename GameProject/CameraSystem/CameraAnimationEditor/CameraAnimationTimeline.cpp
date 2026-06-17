#ifdef _DEBUG

#include "CameraAnimationTimeline.h"
#include <algorithm>
#include <cmath>
#include <format>

CameraAnimationTimeline::CameraAnimationTimeline() {
    // 初期は SUMMARY トラックのみ表示
    for (int i = 0; i < static_cast<int>(TrackType::COUNT); ++i) {
        trackVisible_[i] = (i == 0);
    }
}

CameraAnimationTimeline::~CameraAnimationTimeline() {
}

void CameraAnimationTimeline::Initialize(CameraAnimation* animation) {
    animation_ = animation;
    selectedKeyframes_.clear();
    hoveredKeyframe_ = -1;
}

void CameraAnimationTimeline::Draw() {
    if (!animation_) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    if (contentSize.x < 100 || contentSize.y < 100) {
        ImGui::PopStyleVar(2);
        return;
    }

    if (ImGui::BeginChild("Timeline", ImVec2(contentSize.x, timelineHeight_), true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();

        drawList->AddRectFilled(canvasPos,
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
            IM_COL32(40, 40, 40, 255));

        DrawGrid();

        DrawTimeRuler();

        float yPos = canvasPos.y + rulerHeight_;
        for (int i = 0; i < static_cast<int>(TrackType::COUNT); ++i) {
            if (trackVisible_[i]) {
                DrawTrack(static_cast<TrackType>(i), yPos);
                yPos += trackHeight_;
            }
        }

        DrawPlayhead();

        if (isRectSelecting_) {
            DrawSelectionRect();
        }

        HandleMouseInput();

        HandleKeyboardInput();
    }
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
}

void CameraAnimationTimeline::Update(float deltaTime) {
    // ホバー中のみ拍動アニメの位相を進める
    if (hoveredKeyframe_ >= 0) {
        hoverAnimTime_ += deltaTime * 3.0f;
    }
    else {
        hoverAnimTime_ = 0.0f;
    }

    if (isKeyframePreviewActive_) {
        animation_->SetCurrentTime(previewTime_);
    }
}

void CameraAnimationTimeline::SetTrackVisible(TrackType track, bool visible) {
    int index = static_cast<int>(track);
    if (index >= 0 && index < static_cast<int>(TrackType::COUNT)) {
        trackVisible_[index] = visible;
    }
}

void CameraAnimationTimeline::DrawTimeRuler() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    drawList->AddRectFilled(
        ImVec2(canvasPos.x + trackLabelWidth_, canvasPos.y),
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + rulerHeight_),
        IM_COL32(50, 50, 50, 255));

    float duration = animation_->GetDuration();
    float timeStep = 1.0f / zoom_;

    // ズーム量から見やすい目盛り間隔へ丸める
    if (timeStep < 0.1f) timeStep = 0.1f;
    else if (timeStep < 0.5f) timeStep = 0.5f;
    else if (timeStep < 1.0f) timeStep = 1.0f;
    else if (timeStep < 5.0f) timeStep = 5.0f;
    else timeStep = 10.0f;

    for (float time = 0; time <= duration + timeStep; time += timeStep) {
        float x = TimeToScreenX(time);
        if (x < trackLabelWidth_ || x > canvasSize.x) continue;

        // 主目盛り
        drawList->AddLine(
            ImVec2(canvasPos.x + x, canvasPos.y + rulerHeight_ - 10),
            ImVec2(canvasPos.x + x, canvasPos.y + rulerHeight_),
            IM_COL32(200, 200, 200, 255));

        std::string label = std::format("{:.1f}", time);
        drawList->AddText(
            ImVec2(canvasPos.x + x - 10, canvasPos.y + 2),
            IM_COL32(200, 200, 200, 255),
            label.c_str());

        // 副目盛り（0.1秒刻み）
        if (timeStep >= 1.0f) {
            for (float subTime = time + 0.1f; subTime < time + timeStep && subTime <= duration; subTime += 0.1f) {
                float subX = TimeToScreenX(subTime);
                if (subX < trackLabelWidth_ || subX > canvasSize.x) continue;

                drawList->AddLine(
                    ImVec2(canvasPos.x + subX, canvasPos.y + rulerHeight_ - 5),
                    ImVec2(canvasPos.x + subX, canvasPos.y + rulerHeight_),
                    IM_COL32(100, 100, 100, 255));
            }
        }
    }
}

void CameraAnimationTimeline::DrawGrid() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    // 垂直線（スナップ間隔ごと）
    float timeStep = gridSnapInterval_;
    float duration = animation_->GetDuration();

    for (float time = 0; time <= duration + timeStep; time += timeStep) {
        float x = TimeToScreenX(time);
        if (x < trackLabelWidth_ || x > canvasSize.x) continue;

        drawList->AddLine(
            ImVec2(canvasPos.x + x, canvasPos.y + rulerHeight_),
            ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
            gridColor_, 1.0f);
    }

    // 水平線（トラック境界）
    float yPos = canvasPos.y + rulerHeight_;
    for (int i = 0; i < static_cast<int>(TrackType::COUNT); ++i) {
        if (trackVisible_[i]) {
            drawList->AddLine(
                ImVec2(canvasPos.x + trackLabelWidth_, yPos),
                ImVec2(canvasPos.x + canvasSize.x, yPos),
                gridColor_, 1.0f);
            yPos += trackHeight_;
        }
    }
}

void CameraAnimationTimeline::DrawPlayhead() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    float currentTime = animation_->GetPlaybackTime();
    float x = TimeToScreenX(currentTime);

    if (x >= trackLabelWidth_ && x <= canvasSize.x) {
        drawList->AddLine(
            ImVec2(canvasPos.x + x, canvasPos.y),
            ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
            playheadColor_, 2.0f);

        // 上部のハンドル（三角形）
        ImVec2 handlePoints[3] = {
            ImVec2(canvasPos.x + x - 5, canvasPos.y),
            ImVec2(canvasPos.x + x + 5, canvasPos.y),
            ImVec2(canvasPos.x + x, canvasPos.y + 10)
        };
        drawList->AddTriangleFilled(handlePoints[0], handlePoints[1], handlePoints[2], playheadColor_);
    }
}

void CameraAnimationTimeline::DrawTrack(TrackType trackType, float yPos) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    ImVec2 labelPos = ImVec2(canvasPos.x + 5, yPos + 5);
    drawList->AddText(labelPos, IM_COL32(200, 200, 200, 255), GetTrackName(trackType));

    // ラベル背景
    drawList->AddRectFilled(
        ImVec2(canvasPos.x, yPos),
        ImVec2(canvasPos.x + trackLabelWidth_, yPos + trackHeight_),
        IM_COL32(60, 60, 60, 255));

    // トラック背景
    drawList->AddRectFilled(
        ImVec2(canvasPos.x + trackLabelWidth_, yPos),
        ImVec2(canvasPos.x + canvasSize.x, yPos + trackHeight_),
        IM_COL32(45, 45, 45, 255));

    for (size_t i = 0; i < animation_->GetKeyframeCount(); ++i) {
        const CameraKeyframe& kf = animation_->GetKeyframe(i);
        float x = TimeToScreenX(kf.time);

        if (x < trackLabelWidth_ || x > canvasSize.x) continue;

        bool isSelected = std::find(selectedKeyframes_.begin(),
            selectedKeyframes_.end(),
            static_cast<int>(i)) != selectedKeyframes_.end();
        bool isHovered = (hoveredKeyframe_ == static_cast<int>(i) && hoveredTrack_ == trackType);

        if (trackType == TrackType::SUMMARY) {
            DrawKeyframe(static_cast<int>(i), x, yPos + trackHeight_ / 2, isSelected, isHovered);
        }
        else {
            // TODO: 個別トラックは該当プロパティが変化したキーフレームのみ表示する
            DrawKeyframe(static_cast<int>(i), x, yPos + trackHeight_ / 2, isSelected, isHovered);
        }
    }
}

void CameraAnimationTimeline::DrawKeyframe(int index, float xPos, float yPos, bool isSelected, bool isHovered) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();

    // ホバーで拍動、選択でさらに拡大
    float scale = 1.0f;
    if (isHovered) {
        scale = 1.0f + 0.2f * std::sin(hoverAnimTime_);
    }
    if (isSelected) {
        scale *= 1.1f;
    }

    float size = keyframeSize_ * scale;

    ImU32 color = IM_COL32(150, 150, 255, 255);
    if (isSelected) {
        color = selectedColor_;
    }
    else if (isHovered) {
        color = hoveredColor_;
    }

    ImVec2 center = ImVec2(canvasPos.x + xPos, yPos);

    switch (keyframeStyle_) {
    case KeyframeStyle::DIAMOND: {
        ImVec2 points[4] = {
            ImVec2(center.x, center.y - size),      // 上
            ImVec2(center.x + size, center.y),      // 右
            ImVec2(center.x, center.y + size),      // 下
            ImVec2(center.x - size, center.y)       // 左
        };
        drawList->AddConvexPolyFilled(points, 4, color);
        drawList->AddPolyline(points, 4, IM_COL32(255, 255, 255, 200), ImDrawFlags_Closed, 2.0f);
        break;
    }
    case KeyframeStyle::CIRCLE:
        drawList->AddCircleFilled(center, size, color);
        drawList->AddCircle(center, size, IM_COL32(255, 255, 255, 200), 0, 2.0f);
        break;

    case KeyframeStyle::SQUARE:
        drawList->AddRectFilled(
            ImVec2(center.x - size, center.y - size),
            ImVec2(center.x + size, center.y + size),
            color);
        drawList->AddRect(
            ImVec2(center.x - size, center.y - size),
            ImVec2(center.x + size, center.y + size),
            IM_COL32(255, 255, 255, 200), 0.0f, 0, 2.0f);
        break;

    case KeyframeStyle::TRIANGLE: {
        ImVec2 points[3] = {
            ImVec2(center.x, center.y - size),
            ImVec2(center.x + size, center.y + size),
            ImVec2(center.x - size, center.y + size)
        };
        drawList->AddTriangleFilled(points[0], points[1], points[2], color);
        drawList->AddTriangle(points[0], points[1], points[2],
            IM_COL32(255, 255, 255, 200), 2.0f);
        break;
    }
    }
}

void CameraAnimationTimeline::DrawSelectionRect() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 fillColor = IM_COL32(100, 150, 255, 50);
    ImU32 borderColor = IM_COL32(100, 150, 255, 200);

    drawList->AddRectFilled(dragStartPos_, dragCurrentPos_, fillColor);
    drawList->AddRect(dragStartPos_, dragCurrentPos_, borderColor);
}

void CameraAnimationTimeline::HandleMouseInput() {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    if (mousePos.x < canvasPos.x || mousePos.x > canvasPos.x + canvasSize.x ||
        mousePos.y < canvasPos.y || mousePos.y > canvasPos.y + canvasSize.y) {
        return;
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        float relX = mousePos.x - canvasPos.x;
        float relY = mousePos.y - canvasPos.y;

        // ルーラー上クリックでスクラブ開始
        if (relY < rulerHeight_) {
            isScrubbing_ = true;
            scrubTime_ = ScreenXToTime(relX);
            // キーフレームプレビュー中はスクラブで時刻を上書きしない
            if (isPreviewModeEnabled_ && !isKeyframePreviewActive_) {
                animation_->SetCurrentTime(scrubTime_);
            }
        }
        else if (relX > trackLabelWidth_) {
            // クリック位置のトラックを判定
            float trackY = rulerHeight_;
            for (int i = 0; i < static_cast<int>(TrackType::COUNT); ++i) {
                if (trackVisible_[i]) {
                    if (relY >= trackY && relY < trackY + trackHeight_) {
                        hoveredTrack_ = static_cast<TrackType>(i);
                        break;
                    }
                    trackY += trackHeight_;
                }
            }

            int hitIndex = HitTestKeyframe(relX, relY, hoveredTrack_);

            if (hitIndex >= 0) {
                if (ImGui::GetIO().KeyCtrl) {
                    // Ctrl+クリック：トグル選択
                    auto it = std::find(selectedKeyframes_.begin(),
                        selectedKeyframes_.end(), hitIndex);
                    if (it != selectedKeyframes_.end()) {
                        selectedKeyframes_.erase(it);
                    }
                    else {
                        selectedKeyframes_.push_back(hitIndex);
                    }
                }
                else if (ImGui::GetIO().KeyShift && !selectedKeyframes_.empty()) {
                    // Shift+クリック：直前の選択から範囲選択
                    int lastSelected = selectedKeyframes_.back();
                    int start = std::min<int>(lastSelected, hitIndex);
                    int end = std::max<int>(lastSelected, hitIndex);
                    for (int i = start; i <= end; ++i) {
                        if (std::ranges::find(selectedKeyframes_, i) == selectedKeyframes_.end()) {
                            selectedKeyframes_.push_back(i);
                        }
                    }
                }
                else {
                    // 通常クリック：単一選択
                    selectedKeyframes_.clear();
                    selectedKeyframes_.push_back(hitIndex);
                }

                // プレビュー中は選択キーフレームの時刻へジャンプ
                if (isPreviewModeEnabled_ && hitIndex >= 0 &&
                    hitIndex < static_cast<int>(animation_->GetKeyframeCount())) {
                    const CameraKeyframe& kf = animation_->GetKeyframe(hitIndex);
                    isKeyframePreviewActive_ = true;
                    previewKeyframeIndex_ = hitIndex;
                    previewTime_ = kf.time;
                    animation_->SetCurrentTime(previewTime_);
                }

                isDragging_ = true;
                dragStartPos_ = mousePos;
                dragStartTimes_.clear();
                for (int idx : selectedKeyframes_) {
                    if (idx >= 0 && idx < static_cast<int>(animation_->GetKeyframeCount())) {
                        dragStartTimes_.push_back(animation_->GetKeyframe(idx).time);
                    }
                }
            }
            else {
                // 空白クリックで矩形選択開始
                if (!ImGui::GetIO().KeyCtrl) {
                    selectedKeyframes_.clear();
                }

                if (isPreviewModeEnabled_) {
                    isKeyframePreviewActive_ = false;
                    previewKeyframeIndex_ = -1;
                }

                isRectSelecting_ = true;
                dragStartPos_ = mousePos;
                dragCurrentPos_ = mousePos;
            }
        }
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        if (isScrubbing_) {
            float relX = mousePos.x - canvasPos.x;
            scrubTime_ = ScreenXToTime(relX);
            scrubTime_ = std::max<float>(0.0f, std::min<float>(scrubTime_, animation_->GetDuration()));
            // キーフレームプレビュー中はスクラブで時刻を上書きしない
            if (isPreviewModeEnabled_ && !isKeyframePreviewActive_) {
                animation_->SetCurrentTime(scrubTime_);
            }
        }
        else if (isDragging_) {
            ProcessKeyframeDrag();
        }
        else if (isRectSelecting_) {
            dragCurrentPos_ = mousePos;
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (isRectSelecting_) {
            ProcessRectSelection();
        }

        isScrubbing_ = false;
        isDragging_ = false;
        isRectSelecting_ = false;
        dragStartTimes_.clear();
    }

    // ホバー検出（ドラッグ/矩形選択中は除く）
    if (!isDragging_ && !isRectSelecting_) {
        float relX = mousePos.x - canvasPos.x;
        float relY = mousePos.y - canvasPos.y;

        float trackY = rulerHeight_;
        hoveredTrack_ = TrackType::SUMMARY;
        for (int i = 0; i < static_cast<int>(TrackType::COUNT); ++i) {
            if (trackVisible_[i]) {
                if (relY >= trackY && relY < trackY + trackHeight_) {
                    hoveredTrack_ = static_cast<TrackType>(i);
                    break;
                }
                trackY += trackHeight_;
            }
        }

        hoveredKeyframe_ = HitTestKeyframe(relX, relY, hoveredTrack_);
    }

    // 中ボタンドラッグでパン
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        isPanning_ = true;
        dragStartOffset_ = offset_;
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) && isPanning_) {
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
        offset_ = dragStartOffset_ - delta.x / (100.0f * zoom_);
        ClampOffset();
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
        isPanning_ = false;
    }

    if (ImGui::GetIO().MouseWheel != 0) {
        if (ImGui::GetIO().KeyCtrl) {
            // Ctrl+ホイール：マウス位置を中心にズーム
            float zoomDelta = ImGui::GetIO().MouseWheel > 0 ? 1.2f : 0.8f;
            float newZoom = zoom_ * zoomDelta;

            newZoom = std::max<float>(minZoom_, std::min<float>(maxZoom_, newZoom));

            // ズーム前後でマウス下の時刻が動かないよう offset を補正
            float mouseTime = ScreenXToTime(mousePos.x - canvasPos.x);
            zoom_ = newZoom;
            float newMouseTime = ScreenXToTime(mousePos.x - canvasPos.x);
            offset_ += (newMouseTime - mouseTime);
            ClampOffset();
        }
        else if (ImGui::GetIO().KeyShift) {
            // Shift+ホイール：横スクロール
            offset_ -= ImGui::GetIO().MouseWheel * 0.5f / zoom_;
            ClampOffset();
        }
    }
}

void CameraAnimationTimeline::HandleKeyboardInput() {
    // Delete の削除処理はエディタークラス側で実行
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !selectedKeyframes_.empty()) {
    }

    // Ctrl+A: 全選択
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
        selectedKeyframes_.clear();
        for (size_t i = 0; i < animation_->GetKeyframeCount(); ++i) {
            selectedKeyframes_.push_back(static_cast<int>(i));
        }
    }
}

float CameraAnimationTimeline::TimeToScreenX(float time) const {
    return trackLabelWidth_ + (time - offset_) * 100.0f * zoom_;
}

float CameraAnimationTimeline::ScreenXToTime(float x) const {
    return ((x - trackLabelWidth_) / (100.0f * zoom_)) + offset_;
}

int CameraAnimationTimeline::HitTestKeyframe(float x, float y, TrackType trackType) const {
    // 対象トラックの Y 範囲を算出
    float trackY = rulerHeight_;
    for (int i = 0; i < static_cast<int>(trackType); ++i) {
        if (trackVisible_[i]) {
            trackY += trackHeight_;
        }
    }

    if (y < trackY || y > trackY + trackHeight_) {
        return -1;
    }

    for (size_t i = 0; i < animation_->GetKeyframeCount(); ++i) {
        const CameraKeyframe& kf = animation_->GetKeyframe(i);
        float kfX = TimeToScreenX(kf.time);
        float kfY = trackY + trackHeight_ / 2;

        float dist = std::sqrt((x - kfX) * (x - kfX) + (y - kfY) * (y - kfY));
        if (dist <= keyframeSize_) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void CameraAnimationTimeline::ProcessRectSelection() {
    float left = std::min<float>(dragStartPos_.x, dragCurrentPos_.x);
    float right = std::max<float>(dragStartPos_.x, dragCurrentPos_.x);
    float top = std::min<float>(dragStartPos_.y, dragCurrentPos_.y);
    float bottom = std::max<float>(dragStartPos_.y, dragCurrentPos_.y);

    for (size_t i = 0; i < animation_->GetKeyframeCount(); ++i) {
        const CameraKeyframe& kf = animation_->GetKeyframe(i);
        float kfX = TimeToScreenX(kf.time);

        // 表示中の各トラック上の位置を矩形と判定
        float trackY = rulerHeight_;
        for (int t = 0; t < static_cast<int>(TrackType::COUNT); ++t) {
            if (trackVisible_[t]) {
                float kfY = trackY + trackHeight_ / 2;

                if (kfX >= left && kfX <= right && kfY >= top && kfY <= bottom) {
                    if (std::find(selectedKeyframes_.begin(),
                        selectedKeyframes_.end(),
                        static_cast<int>(i)) == selectedKeyframes_.end()) {
                        selectedKeyframes_.push_back(static_cast<int>(i));
                    }
                    break;
                }
                trackY += trackHeight_;
            }
        }
    }
}

void CameraAnimationTimeline::ProcessKeyframeDrag() {
    ImVec2 mousePos = ImGui::GetMousePos();
    float deltaX = mousePos.x - dragStartPos_.x;
    float deltaTime = deltaX / (100.0f * zoom_);

    for (size_t i = 0; i < selectedKeyframes_.size(); ++i) {
        int idx = selectedKeyframes_[i];
        if (idx >= 0 && idx < static_cast<int>(animation_->GetKeyframeCount()) &&
            i < dragStartTimes_.size()) {

            CameraKeyframe kf = animation_->GetKeyframe(idx);
            float newTime = dragStartTimes_[i] + deltaTime;

            if (enableGridSnap_) {
                newTime = SnapToGrid(newTime);
            }

            newTime = std::max<float>(0.0f, std::min<float>(newTime, animation_->GetDuration()));

            kf.time = newTime;
            animation_->EditKeyframe(idx, kf);

            // プレビュー中のキーフレームを動かしたら時刻も追従
            if (isPreviewModeEnabled_ && isKeyframePreviewActive_ && idx == previewKeyframeIndex_) {
                previewTime_ = newTime;
                animation_->SetCurrentTime(previewTime_);
            }
        }
    }
}

float CameraAnimationTimeline::SnapToGrid(float time) const {
    if (!enableGridSnap_) return time;
    return std::round(time / gridSnapInterval_) * gridSnapInterval_;
}

const char* CameraAnimationTimeline::GetTrackName(TrackType track) const {
    switch (track) {
    case TrackType::SUMMARY: return "Summary";
    case TrackType::POSITION_X: return "Pos X";
    case TrackType::POSITION_Y: return "Pos Y";
    case TrackType::POSITION_Z: return "Pos Z";
    case TrackType::ROTATION_X: return "Rot X";
    case TrackType::ROTATION_Y: return "Rot Y";
    case TrackType::ROTATION_Z: return "Rot Z";
    case TrackType::FOV: return "FOV";
    default: return "Unknown";
    }
}

ImU32 CameraAnimationTimeline::GetTrackColor(TrackType track) const {
    switch (track) {
    case TrackType::SUMMARY: return IM_COL32(200, 200, 200, 255);
    case TrackType::POSITION_X: return IM_COL32(255, 100, 100, 255);
    case TrackType::POSITION_Y: return IM_COL32(100, 255, 100, 255);
    case TrackType::POSITION_Z: return IM_COL32(100, 100, 255, 255);
    case TrackType::ROTATION_X: return IM_COL32(255, 200, 100, 255);
    case TrackType::ROTATION_Y: return IM_COL32(200, 255, 100, 255);
    case TrackType::ROTATION_Z: return IM_COL32(100, 200, 255, 255);
    case TrackType::FOV: return IM_COL32(255, 255, 100, 255);
    default: return IM_COL32(150, 150, 150, 255);
    }
}

void CameraAnimationTimeline::SetZoom(float zoom) {
    zoom_ = std::max<float>(minZoom_, std::min<float>(maxZoom_, zoom));
    ClampOffset();
}

void CameraAnimationTimeline::SetOffset(float offset) {
    offset_ = offset;
    ClampOffset();
}

void CameraAnimationTimeline::ClampOffset() {
    if (!animation_) return;

    // キャンバス幅を 800px と仮定
    float visibleWidth = 800.0f - trackLabelWidth_;
    float visibleTime = visibleWidth / (100.0f * zoom_);
    float duration = animation_->GetDuration();

    // 開始より前は見せない
    offset_ = std::max<float>(-1.0f, offset_);

    if (visibleTime >= duration + 2.0f) {
        // 全体が収まる場合は左寄せ
        offset_ = 0.0f;
    }
    else {
        // 終端より先を見せすぎない
        offset_ = std::min<float>(offset_, duration - visibleTime + 1.0f);
    }
}

#endif // _DEBUG