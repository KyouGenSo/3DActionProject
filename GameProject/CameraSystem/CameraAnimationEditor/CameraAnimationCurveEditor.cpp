#ifdef _DEBUG

#include "CameraAnimationCurveEditor.h"
#include "EaseFunc.h"
#include <algorithm>
#include <cmath>
#include <format>

CameraAnimationCurveEditor::CameraAnimationCurveEditor() {
    // カーブ色の初期化
    curveColors_[static_cast<int>(CurveType::POSITION_X)] = IM_COL32(255, 100, 100, 255);
    curveColors_[static_cast<int>(CurveType::POSITION_Y)] = IM_COL32(100, 255, 100, 255);
    curveColors_[static_cast<int>(CurveType::POSITION_Z)] = IM_COL32(100, 100, 255, 255);
    curveColors_[static_cast<int>(CurveType::ROTATION_X)] = IM_COL32(255, 200, 100, 255);
    curveColors_[static_cast<int>(CurveType::ROTATION_Y)] = IM_COL32(200, 255, 100, 255);
    curveColors_[static_cast<int>(CurveType::ROTATION_Z)] = IM_COL32(100, 200, 255, 255);
    curveColors_[static_cast<int>(CurveType::FOV)] = IM_COL32(255, 255, 100, 255);

    // 初期状態では POSITION_X のみ表示（タブ選択で切り替わる）
    for (int i = 0; i < static_cast<int>(CurveType::COUNT); ++i) {
        curveVisible_[i] = (i == 0);  // POSITION_X のみ
    }
}

CameraAnimationCurveEditor::~CameraAnimationCurveEditor() {
}

void CameraAnimationCurveEditor::Initialize(CameraAnimation* animation) {
    animation_ = animation;
    tangents_.clear();
    selectedKeyPoint_ = -1;
}

void CameraAnimationCurveEditor::Draw(const std::vector<int>& selectedKeyframes) {
    if (!animation_) return;

    ImGui::Text("Curve Editor");
    ImGui::Separator();

    // カーブ選択タブ
    const char* curveNames[] = { "Pos X", "Pos Y", "Pos Z", "Rot X", "Rot Y", "Rot Z", "FOV" };
    static int selectedTab = 0;
    if (ImGui::BeginTabBar("CurveTypes")) {
        for (int i = 0; i < static_cast<int>(CurveType::COUNT); ++i) {
            if (ImGui::BeginTabItem(curveNames[i])) {
                selectedTab = i;
                SetActiveCurve(static_cast<CurveType>(i));
                // 選択されたカーブのみ可視に設定
                for (int j = 0; j < static_cast<int>(CurveType::COUNT); ++j) {
                    SetCurveVisible(static_cast<CurveType>(j), j == i);
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    // イージングプリセット
    DrawEasingPresets();

    // グラフエリア
    DrawGraphArea();

    // カーブプロパティ
    DrawCurveProperties();
}

void CameraAnimationCurveEditor::DrawGraphArea() {
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 100 || canvasSize.y < 100) {
        canvasSize = ImVec2(600, 300);
    }
    graphSize_ = ImVec2(canvasSize.x - 20, std::min<float>(300.0f, canvasSize.y - 100.0f));

    ImGui::BeginChild("GraphArea", graphSize_, true, ImGuiWindowFlags_NoScrollbar);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    graphPos_ = ImGui::GetCursorScreenPos();
    ImVec2 graphMax = ImVec2(graphPos_.x + graphSize_.x, graphPos_.y + graphSize_.y);

    // 背景
    drawList->AddRectFilled(graphPos_, graphMax, IM_COL32(30, 30, 30, 255));

    // グリッド描画
    if (showGrid_) {
        DrawGrid();
    }

    // 軸描画
    if (showAxes_) {
        DrawAxes();
    }

    // カーブ描画
    for (int i = 0; i < static_cast<int>(CurveType::COUNT); ++i) {
        if (curveVisible_[i]) {
            DrawCurve(static_cast<CurveType>(i));
        }
    }

    // マウス入力処理
    HandleMouseInput();

    ImGui::EndChild();
}

void CameraAnimationCurveEditor::DrawGrid() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // 時間グリッド（垂直線）
    float timeStep = 1.0f;  // 1秒ごと
    for (float t = 0; t <= timeRange_; t += timeStep) {
        ImVec2 p = ValueToGraph(t, 0);
        if (p.x >= graphPos_.x && p.x <= graphPos_.x + graphSize_.x) {
            drawList->AddLine(
                ImVec2(p.x, graphPos_.y),
                ImVec2(p.x, graphPos_.y + graphSize_.y),
                IM_COL32(60, 60, 60, 255));
        }
    }

    // 値グリッド（水平線）
    float valueStep = (valueRangeMax_ - valueRangeMin_) / 10.0f;
    for (float v = valueRangeMin_; v <= valueRangeMax_; v += valueStep) {
        ImVec2 p = ValueToGraph(0, v);
        if (p.y >= graphPos_.y && p.y <= graphPos_.y + graphSize_.y) {
            drawList->AddLine(
                ImVec2(graphPos_.x, p.y),
                ImVec2(graphPos_.x + graphSize_.x, p.y),
                IM_COL32(60, 60, 60, 255));
        }
    }
}

void CameraAnimationCurveEditor::DrawAxes() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // X 軸（時間）
    ImVec2 xAxisStart = ValueToGraph(0, 0);
    ImVec2 xAxisEnd = ValueToGraph(timeRange_, 0);
    if (xAxisStart.y >= graphPos_.y && xAxisStart.y <= graphPos_.y + graphSize_.y) {
        drawList->AddLine(xAxisStart, xAxisEnd, IM_COL32(100, 100, 100, 255), 2.0f);
    }

    // Y 軸（値）
    ImVec2 yAxisStart = ValueToGraph(0, valueRangeMin_);
    ImVec2 yAxisEnd = ValueToGraph(0, valueRangeMax_);
    if (yAxisStart.x >= graphPos_.x && yAxisStart.x <= graphPos_.x + graphSize_.x) {
        drawList->AddLine(yAxisStart, yAxisEnd, IM_COL32(100, 100, 100, 255), 2.0f);
    }
}

void CameraAnimationCurveEditor::DrawCurve(CurveType curveType) {
    if (!animation_ || animation_->GetKeyframeCount() < 2) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 color = curveColors_[static_cast<int>(curveType)];

    // キーフレーム間の補間カーブを描画
    std::vector<ImVec2> points;

    for (size_t i = 0; i < animation_->GetKeyframeCount() - 1; ++i) {
        const CameraKeyframe& kf1 = animation_->GetKeyframe(i);
        const CameraKeyframe& kf2 = animation_->GetKeyframe(i + 1);

        float v1 = GetCurveValue(kf1, curveType);
        float v2 = GetCurveValue(kf2, curveType);

        // 補間曲線を描画（解像度分のポイントを生成）
        for (int j = 0; j <= curveResolution_; ++j) {
            float t = static_cast<float>(j) / curveResolution_;
            float time = kf1.time + (kf2.time - kf1.time) * t;

            // イージング適用
            float easedT = ApplyEasing(t, kf1.interpolation);
            float value = v1 + (v2 - v1) * easedT;

            ImVec2 p = ValueToGraph(time, value);
            if (p.x >= graphPos_.x && p.x <= graphPos_.x + graphSize_.x &&
                p.y >= graphPos_.y && p.y <= graphPos_.y + graphSize_.y) {
                points.push_back(p);
            }
        }
    }

    // カーブを描画
    if (points.size() > 1) {
        for (size_t i = 0; i < points.size() - 1; ++i) {
            drawList->AddLine(points[i], points[i + 1], color, 2.0f);
        }
    }

    // キーポイントを描画
    for (size_t i = 0; i < animation_->GetKeyframeCount(); ++i) {
        const CameraKeyframe& kf = animation_->GetKeyframe(i);
        float value = GetCurveValue(kf, curveType);
        ImVec2 p = ValueToGraph(kf.time, value);

        if (p.x >= graphPos_.x - 5 && p.x <= graphPos_.x + graphSize_.x + 5 &&
            p.y >= graphPos_.y - 5 && p.y <= graphPos_.y + graphSize_.y + 5) {

            bool isSelected = (selectedKeyPoint_ == static_cast<int>(i) &&
                activeCurve_ == curveType);
            DrawKeyPoint(static_cast<int>(i), p.x, p.y, isSelected);
        }
    }
}

void CameraAnimationCurveEditor::DrawKeyPoint(int index, float x, float y, bool isSelected) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float radius = isSelected ? 6.0f : 4.0f;
    ImU32 fillColor = isSelected ? IM_COL32(255, 200, 100, 255) : IM_COL32(255, 255, 255, 255);
    ImU32 borderColor = IM_COL32(0, 0, 0, 255);

    drawList->AddCircleFilled(ImVec2(x, y), radius, fillColor);
    drawList->AddCircle(ImVec2(x, y), radius, borderColor, 0, 2.0f);

    // 値表示
    if (showValues_ && isSelected) {
        const CameraKeyframe& kf = animation_->GetKeyframe(index);
        float value = GetCurveValue(kf, activeCurve_);
        std::string label = std::format("{:.2f}", value);
        drawList->AddText(ImVec2(x + 10, y - 10), IM_COL32(255, 255, 255, 255), label.c_str());
    }
}

void CameraAnimationCurveEditor::DrawEasingPresets() {
    ImGui::Text("Easing Presets:");
    ImGui::SameLine();

    const char* easingNames[] = { "Linear", "Ease In", "Ease Out", "Ease In-Out" };

    // ドロップダウンメニュー（選択のみ、適用はボタンで行う）
    ImGui::PushItemWidth(120.0f);
    ImGui::Combo("##EasingPreset", &selectedEasingIndex_, easingNames, 4);
    ImGui::PopItemWidth();

    // 適用ボタン
    ImGui::SameLine();
    if (ImGui::Button("Apply")) {
        if (selectedKeyPoint_ >= 0 && selectedKeyPoint_ < static_cast<int>(animation_->GetKeyframeCount())) {
            CameraKeyframe kf = animation_->GetKeyframe(selectedKeyPoint_);
            kf.interpolation = static_cast<CameraKeyframe::InterpolationType>(selectedEasingIndex_);
            animation_->EditKeyframe(selectedKeyPoint_, kf);
        }
    }
}

void CameraAnimationCurveEditor::DrawCurveProperties() {
    ImGui::Separator();
    ImGui::Text("Curve Properties");

    // 表示設定
    ImGui::Checkbox("Show Grid", &showGrid_);
    ImGui::SameLine();
    ImGui::Checkbox("Show Axes", &showAxes_);
    ImGui::SameLine();
    ImGui::Checkbox("Show Values", &showValues_);

    // ズーム・パン
    ImGui::DragFloat("Zoom X", &zoomX_, 0.01f, 0.1f, 10.0f);
    ImGui::DragFloat("Zoom Y", &zoomY_, 0.01f, 0.1f, 10.0f);

    // 範囲設定
    ImGui::DragFloat("Time Range", &timeRange_, 0.1f, 1.0f, 60.0f);
    ImGui::DragFloat2("Value Range", &valueRangeMin_, 0.1f);
}

void CameraAnimationCurveEditor::HandleMouseInput() {
    ImVec2 mousePos = ImGui::GetMousePos();

    // グラフエリア内かチェック
    if (mousePos.x < graphPos_.x || mousePos.x > graphPos_.x + graphSize_.x ||
        mousePos.y < graphPos_.y || mousePos.y > graphPos_.y + graphSize_.y) {
        return;
    }

    // クリック処理
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        // キーポイントのヒットテスト
        selectedKeyPoint_ = -1;
        for (size_t i = 0; i < animation_->GetKeyframeCount(); ++i) {
            const CameraKeyframe& kf = animation_->GetKeyframe(i);
            float value = GetCurveValue(kf, activeCurve_);
            ImVec2 p = ValueToGraph(kf.time, value);

            float dist = std::sqrt(
                (mousePos.x - p.x) * (mousePos.x - p.x) +
                (mousePos.y - p.y) * (mousePos.y - p.y));

            if (dist <= 6.0f) {
                selectedKeyPoint_ = static_cast<int>(i);
                isDragging_ = true;
                dragStartPos_ = mousePos;
                dragStartTime_ = kf.time;
                dragStartValue_ = value;
                break;
            }
        }
    }

    // ドラッグ処理
    if (isDragging_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        if (selectedKeyPoint_ >= 0 && selectedKeyPoint_ < static_cast<int>(animation_->GetKeyframeCount())) {
            float time, value;
            GraphToValue(mousePos, time, value);

            // グリッドスナップ
            if (enableGridSnap_) {
                SnapToGrid(time, value);
            }

            // 時間と値の制限
            time = std::max<float>(0.0f, std::min<float>(time, animation_->GetDuration()));
            value = std::max<float>(valueRangeMin_, std::min<float>(value, valueRangeMax_));

            // キーフレームを更新
            CameraKeyframe kf = animation_->GetKeyframe(selectedKeyPoint_);
            kf.time = time;
            SetCurveValue(kf, activeCurve_, value);
            animation_->EditKeyframe(selectedKeyPoint_, kf);
        }
    }

    // リリース処理
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        isDragging_ = false;
    }

    // ホイール（ズーム）
    if (ImGui::GetIO().MouseWheel != 0) {
        float zoomDelta = ImGui::GetIO().MouseWheel * 0.1f;
        zoomX_ = std::max<float>(0.1f, std::min<float>(10.0f, zoomX_ + zoomDelta));
        zoomY_ = std::max<float>(0.1f, std::min<float>(10.0f, zoomY_ + zoomDelta));
    }
}

ImVec2 CameraAnimationCurveEditor::ValueToGraph(float time, float value) const {
    float x = graphPos_.x + (time / timeRange_) * graphSize_.x * zoomX_ + panX_;
    float y = graphPos_.y + graphSize_.y -
        ((value - valueRangeMin_) / (valueRangeMax_ - valueRangeMin_)) * graphSize_.y * zoomY_ - panY_;
    return ImVec2(x, y);
}

void CameraAnimationCurveEditor::GraphToValue(const ImVec2& pos, float& time, float& value) const {
    time = ((pos.x - graphPos_.x - panX_) / (graphSize_.x * zoomX_)) * timeRange_;
    value = valueRangeMin_ +
        (1.0f - (pos.y - graphPos_.y + panY_) / (graphSize_.y * zoomY_)) *
        (valueRangeMax_ - valueRangeMin_);
}

float CameraAnimationCurveEditor::GetCurveValue(const CameraKeyframe& kf, CurveType type) const {
    switch (type) {
    case CurveType::POSITION_X: return kf.position.x;
    case CurveType::POSITION_Y: return kf.position.y;
    case CurveType::POSITION_Z: return kf.position.z;
    case CurveType::ROTATION_X: return kf.rotation.x;
    case CurveType::ROTATION_Y: return kf.rotation.y;
    case CurveType::ROTATION_Z: return kf.rotation.z;
    case CurveType::FOV: return kf.fov;
    default: return 0.0f;
    }
}

void CameraAnimationCurveEditor::SetCurveValue(CameraKeyframe& kf, CurveType type, float value) {
    switch (type) {
    case CurveType::POSITION_X: kf.position.x = value; break;
    case CurveType::POSITION_Y: kf.position.y = value; break;
    case CurveType::POSITION_Z: kf.position.z = value; break;
    case CurveType::ROTATION_X: kf.rotation.x = value; break;
    case CurveType::ROTATION_Y: kf.rotation.y = value; break;
    case CurveType::ROTATION_Z: kf.rotation.z = value; break;
    case CurveType::FOV: kf.fov = value; break;
    }
}

float CameraAnimationCurveEditor::ApplyEasing(float t, CameraKeyframe::InterpolationType type) const {
    switch (type) {
    case CameraKeyframe::InterpolationType::LINEAR:
        return Tako::Ease::Linear(t);

    case CameraKeyframe::InterpolationType::EASE_IN:
        return Tako::Ease::InQuad(t);

    case CameraKeyframe::InterpolationType::EASE_OUT:
        return Tako::Ease::OutQuad(t);

    case CameraKeyframe::InterpolationType::EASE_IN_OUT:
        return Tako::Ease::InOutQuad(t);

    default:
        return t;
    }
}

void CameraAnimationCurveEditor::SnapToGrid(float& time, float& value) const {
    time = std::round(time / gridSnapIntervalX_) * gridSnapIntervalX_;
    value = std::round(value / gridSnapIntervalY_) * gridSnapIntervalY_;
}

void CameraAnimationCurveEditor::SetCurveVisible(CurveType type, bool visible) {
    int index = static_cast<int>(type);
    if (index >= 0 && index < static_cast<int>(CurveType::COUNT)) {
        curveVisible_[index] = visible;
    }
}

#endif // _DEBUG