/*
 * BenchmarkWindow.cpp - GUI Implementation for Performance Benchmark
 */

#include "BenchmarkWindow.h"
#include <LayoutBuilder.h>
#include <GroupLayout.h>
#include <GridLayout.h>
#include <Alert.h>
#include <File.h>
#include <Path.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Catalog.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <string.h>
#include <kernel/OS.h>
#include <time.h>
#include <Directory.h>
#include <fs_attr.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BenchmarkWindow"

namespace HaikuDAW {

// BenchmarkGraphView Implementation
BenchmarkGraphView::BenchmarkGraphView(BRect frame, const char* name)
    : BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
    , fProgress(0.0f)
    , fOffscreenBitmap(nullptr)
    , fOffscreenView(nullptr)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

BenchmarkGraphView::~BenchmarkGraphView()
{
    delete fOffscreenBitmap;
}

void BenchmarkGraphView::AttachedToWindow()
{
    BView::AttachedToWindow();
    
    // Create offscreen bitmap for smooth drawing
    BRect bounds = Bounds();
    fOffscreenBitmap = new BBitmap(bounds, B_RGB32, true);
    fOffscreenView = new BView(bounds, "offscreen", B_FOLLOW_ALL, B_WILL_DRAW);
    fOffscreenBitmap->AddChild(fOffscreenView);
    
    // Enable anti-aliasing for smoother graphics (Haiku native)
    fOffscreenBitmap->Lock();
    fOffscreenView->SetDrawingMode(B_OP_ALPHA);
    fOffscreenView->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
    fOffscreenView->SetFlags(fOffscreenView->Flags() | B_SUBPIXEL_PRECISE);
    fOffscreenBitmap->Unlock();
}

void BenchmarkGraphView::Draw(BRect updateRect)
{
    if (!fOffscreenBitmap) return;
    
    // Draw to offscreen bitmap
    fOffscreenBitmap->Lock();
    fOffscreenView->SetHighColor(ViewColor());
    fOffscreenView->FillRect(fOffscreenView->Bounds());
    
    BRect bounds = Bounds();
    
    // Draw modern header with gradient effect
    DrawHeader(bounds);
    
    // Layout areas for different visualizations
    float headerHeight = 35;
    float margin = 10;
    float halfWidth = (bounds.Width() - margin * 3) / 2;
    float halfHeight = (bounds.Height() - headerHeight - margin * 3) / 2;
    
    // Top left: Live bar chart
    BRect chartArea(margin, headerHeight + margin, 
                   margin + halfWidth, headerHeight + margin + halfHeight);
    if (!fResults.empty()) {
        DrawModernBarChart(chartArea);
    } else {
        DrawPlaceholder(chartArea, "Bar Chart - Waiting for results...");
    }
    
    // Top right: Category pie chart
    BRect pieArea(margin * 2 + halfWidth, headerHeight + margin,
                 bounds.Width() - margin, headerHeight + margin + halfHeight);
    if (!fCategoryScores.empty()) {
        DrawModernPieChart(pieArea);
    } else {
        DrawPlaceholder(pieArea, "Category Breakdown - Waiting for results...");
    }
    
    // Bottom left: Performance meter
    BRect meterArea(margin, headerHeight + margin * 2 + halfHeight,
                   margin + halfWidth, bounds.Height() - margin - 40);
    DrawPerformanceMeter(meterArea);
    
    // Bottom right: Live stats
    BRect statsArea(margin * 2 + halfWidth, headerHeight + margin * 2 + halfHeight,
                   bounds.Width() - margin, bounds.Height() - margin - 40);
    DrawLiveStats(statsArea);
    
    // Bottom: Progress bar
    if (fProgress > 0.0f) {
        BRect progressRect(margin, bounds.Height() - 35, bounds.Width() - margin, bounds.Height() - 10);
        DrawModernProgressBar(progressRect, fProgress);
    }
    
    fOffscreenView->Sync();
    fOffscreenBitmap->Unlock();
    
    // Copy to screen
    DrawBitmap(fOffscreenBitmap, BPoint(0, 0));
}

void BenchmarkGraphView::DrawBarChart(BRect bounds)
{
    if (fResults.empty()) return;
    
    float barWidth = bounds.Width() / fResults.size();
    float maxScore = 100.0f;
    
    fOffscreenView->SetHighColor(0, 0, 0);
    fOffscreenView->StrokeRect(bounds);
    
    for (size_t i = 0; i < fResults.size(); i++) {
        float score = fResults[i].score;
        float barHeight = (score / maxScore) * bounds.Height();
        
        BRect barRect(
            bounds.left + i * barWidth + 2,
            bounds.bottom - barHeight,
            bounds.left + (i + 1) * barWidth - 2,
            bounds.bottom
        );
        
        // Color based on score
        if (score >= 90) fOffscreenView->SetHighColor(0, 200, 0);      // Green
        else if (score >= 75) fOffscreenView->SetHighColor(0, 150, 200); // Blue
        else if (score >= 50) fOffscreenView->SetHighColor(255, 200, 0); // Yellow
        else fOffscreenView->SetHighColor(255, 100, 100);                // Red
        
        fOffscreenView->FillRect(barRect);
        
        // Draw score text
        char scoreText[32];
        snprintf(scoreText, sizeof(scoreText), "%.1f", score);
        fOffscreenView->SetHighColor(0, 0, 0);
        fOffscreenView->SetFontSize(9);
        fOffscreenView->DrawString(scoreText, 
            BPoint(barRect.left + 2, bounds.bottom - barHeight - 2));
    }
}

void BenchmarkGraphView::DrawCategoryPie(BRect bounds)
{
    if (fCategoryScores.empty()) return;
    
    BPoint center((bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2);
    float radius = std::min(bounds.Width(), bounds.Height()) / 2 - 10;
    
    float startAngle = 0;
    float total = 0;
    for (const auto& cat : fCategoryScores) {
        total += cat.second;
    }
    
    int colorIndex = 0;
    rgb_color colors[] = {
        {200, 100, 100}, // Red
        {100, 200, 100}, // Green
        {100, 100, 200}, // Blue
        {200, 200, 100}, // Yellow
        {200, 100, 200}, // Magenta
        {100, 200, 200}  // Cyan
    };
    
    for (const auto& cat : fCategoryScores) {
        float angle = (cat.second / total) * 360.0f;
        
        fOffscreenView->SetHighColor(colors[colorIndex % 6]);
        fOffscreenView->FillArc(center, radius, radius, startAngle, angle);
        
        // Draw category label
        float midAngle = startAngle + angle / 2;
        float labelX = center.x + cos(midAngle * M_PI / 180) * radius * 0.7;
        float labelY = center.y + sin(midAngle * M_PI / 180) * radius * 0.7;
        
        fOffscreenView->SetHighColor(0, 0, 0);
        fOffscreenView->SetFontSize(10);
        fOffscreenView->DrawString(cat.first.c_str(), BPoint(labelX - 20, labelY));
        
        startAngle += angle;
        colorIndex++;
    }
}

void BenchmarkGraphView::DrawProgressBar(BRect rect, const char* label, float value, float max)
{
    float percentage = (value / max) * 100.0f;
    float filled = (percentage / 100.0f) * rect.Width();
    
    // Background
    fOffscreenView->SetHighColor(200, 200, 200);
    fOffscreenView->FillRect(rect);
    
    // Progress fill
    BRect fillRect = rect;
    fillRect.right = fillRect.left + filled;
    
    if (percentage >= 90) fOffscreenView->SetHighColor(0, 200, 0);
    else if (percentage >= 75) fOffscreenView->SetHighColor(0, 150, 200);
    else if (percentage >= 50) fOffscreenView->SetHighColor(255, 200, 0);
    else fOffscreenView->SetHighColor(255, 100, 100);
    
    fOffscreenView->FillRect(fillRect);
    
    // Border
    fOffscreenView->SetHighColor(0, 0, 0);
    fOffscreenView->StrokeRect(rect);
    
    // Label
    char text[64];
    snprintf(text, sizeof(text), "%s: %.1f%%", label, percentage);
    fOffscreenView->DrawString(text, BPoint(rect.left + 5, rect.top - 2));
}

void BenchmarkGraphView::SetData(const std::vector<BenchmarkResult>& results)
{
    fResults = results;
    Invalidate();
}

void BenchmarkGraphView::SetCategoryData(const std::map<std::string, float>& scores)
{
    fCategoryScores = scores;
    Invalidate();
}

void BenchmarkGraphView::UpdateProgress(float progress)
{
    fProgress = progress;
    Invalidate();
}

void BenchmarkGraphView::FrameResized(float width, float height)
{
    // Recreate offscreen bitmap
    delete fOffscreenBitmap;
    
    BRect bounds(0, 0, width, height);
    fOffscreenBitmap = new BBitmap(bounds, B_RGB32, true);
    fOffscreenView = new BView(bounds, "offscreen", B_FOLLOW_ALL, B_WILL_DRAW);
    fOffscreenBitmap->AddChild(fOffscreenView);
    
    Invalidate();
}

void BenchmarkGraphView::DrawHeader(BRect bounds)
{
    // Draw gradient header
    for (int i = 0; i < 35; i++) {
        int gray = 100 - (i * 2); // Gradient from dark to light
        fOffscreenView->SetHighColor(gray, gray, gray + 20);
        fOffscreenView->StrokeLine(BPoint(0, i), BPoint(bounds.Width(), i));
    }
    
    // Draw title
    fOffscreenView->SetHighColor(255, 255, 255);
    fOffscreenView->SetFontSize(16);
    font_height fh;
    fOffscreenView->GetFontHeight(&fh);
    float textY = 20 + fh.ascent / 2;
    fOffscreenView->DrawString("HaikuMix Performance Dashboard", BPoint(15, textY));
    
    // Draw timestamp
    char timeStr[64];
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm);
    fOffscreenView->SetFontSize(11);
    float timeWidth = fOffscreenView->StringWidth(timeStr);
    fOffscreenView->DrawString(timeStr, BPoint(bounds.Width() - timeWidth - 15, textY));
}

void BenchmarkGraphView::DrawPlaceholder(BRect area, const char* text)
{
    // Draw border
    fOffscreenView->SetHighColor(200, 200, 200);
    fOffscreenView->StrokeRect(area);
    
    // Draw centered text
    fOffscreenView->SetHighColor(150, 150, 150);
    fOffscreenView->SetFontSize(10);
    float textWidth = fOffscreenView->StringWidth(text);
    float x = area.left + (area.Width() - textWidth) / 2;
    float y = area.top + area.Height() / 2;
    fOffscreenView->DrawString(text, BPoint(x, y));
}

void BenchmarkGraphView::DrawModernBarChart(BRect area)
{
    // Draw background
    fOffscreenView->SetHighColor(245, 245, 245);
    fOffscreenView->FillRect(area);
    
    // Draw border
    fOffscreenView->SetHighColor(180, 180, 180);
    fOffscreenView->StrokeRect(area);
    
    if (fResults.empty()) return;
    
    // Calculate bar dimensions
    float padding = 10;
    float chartWidth = area.Width() - padding * 2;
    float chartHeight = area.Height() - padding * 2 - 20;
    float barWidth = chartWidth / fResults.size();
    float maxScore = 100.0f;
    
    // Draw bars with 3D effect
    for (size_t i = 0; i < fResults.size(); i++) {
        float score = fResults[i].score;
        float barHeight = (score / maxScore) * chartHeight;
        
        float x = area.left + padding + i * barWidth;
        float y = area.bottom - padding - barHeight;
        float w = barWidth * 0.8f;
        
        // Draw shadow
        fOffscreenView->SetHighColor(100, 100, 100, 50);
        BRect shadow(x + 2, y + 2, x + w + 2, area.bottom - padding + 2);
        fOffscreenView->FillRect(shadow);
        
        // Draw bar with gradient effect using BGradient (Haiku native)
        rgb_color topColor = GetScoreGradientColor(score);
        rgb_color bottomColor = {
            (uint8)(topColor.red * 0.7),
            (uint8)(topColor.green * 0.7),
            (uint8)(topColor.blue * 0.7),
            255
        };
        
        // Vertical gradient fill
        BRect bar(x, y, x + w, area.bottom - padding);
        for (int gy = 0; gy < bar.Height(); gy++) {
            float t = gy / bar.Height();
            rgb_color gradColor = {
                (uint8)(topColor.red + (bottomColor.red - topColor.red) * t),
                (uint8)(topColor.green + (bottomColor.green - topColor.green) * t),
                (uint8)(topColor.blue + (bottomColor.blue - topColor.blue) * t),
                255
            };
            fOffscreenView->SetHighColor(gradColor);
            fOffscreenView->StrokeLine(
                BPoint(bar.left, bar.top + gy),
                BPoint(bar.right, bar.top + gy)
            );
        }
        
        // Draw highlight
        fOffscreenView->SetHighColor(255, 255, 255, 100);
        fOffscreenView->StrokeLine(BPoint(x, y), BPoint(x + w, y));
        fOffscreenView->StrokeLine(BPoint(x, y), BPoint(x, area.bottom - padding));
        
        // Draw score text
        char scoreText[32];
        snprintf(scoreText, sizeof(scoreText), "%.0f", score);
        fOffscreenView->SetHighColor(0, 0, 0);
        fOffscreenView->SetFontSize(9);
        float textWidth = fOffscreenView->StringWidth(scoreText);
        fOffscreenView->DrawString(scoreText, 
            BPoint(x + (w - textWidth) / 2, y - 2));
    }
    
    // Draw title
    fOffscreenView->SetHighColor(0, 0, 0);
    fOffscreenView->SetFontSize(11);
    fOffscreenView->DrawString("Test Scores", BPoint(area.left + 5, area.top + 15));
}

void BenchmarkGraphView::DrawModernPieChart(BRect area)
{
    // Draw background
    fOffscreenView->SetHighColor(245, 245, 245);
    fOffscreenView->FillRect(area);
    
    // Draw border
    fOffscreenView->SetHighColor(180, 180, 180);
    fOffscreenView->StrokeRect(area);
    
    if (fCategoryScores.empty()) return;
    
    BPoint center((area.left + area.right) / 2, (area.top + area.bottom) / 2);
    float radius = std::min(area.Width(), area.Height()) / 2 - 20;
    
    // Calculate total
    float total = 0;
    for (const auto& cat : fCategoryScores) {
        total += cat.second;
    }
    
    // Draw pie slices with 3D effect
    float startAngle = 0;
    int colorIndex = 0;
    
    for (const auto& cat : fCategoryScores) {
        float angle = (cat.second / total) * 360.0f;
        
        // Draw shadow (offset pie)
        fOffscreenView->SetHighColor(100, 100, 100, 50);
        BPoint shadowCenter(center.x + 3, center.y + 3);
        fOffscreenView->FillArc(shadowCenter, radius, radius, startAngle, angle);
        
        // Draw main slice
        rgb_color sliceColor = GetCategoryColor(colorIndex);
        fOffscreenView->SetHighColor(sliceColor);
        fOffscreenView->FillArc(center, radius, radius, startAngle, angle);
        
        // Draw border
        fOffscreenView->SetHighColor(255, 255, 255);
        fOffscreenView->SetPenSize(2);
        fOffscreenView->StrokeArc(center, radius, radius, startAngle, angle);
        fOffscreenView->SetPenSize(1);
        
        // Draw label
        float midAngle = startAngle + angle / 2;
        float labelRadius = radius * 0.7f;
        float labelX = center.x + cosf(midAngle * M_PI / 180) * labelRadius;
        float labelY = center.y + sinf(midAngle * M_PI / 180) * labelRadius;
        
        fOffscreenView->SetHighColor(255, 255, 255);
        fOffscreenView->SetFontSize(10);
        char label[64];
        snprintf(label, sizeof(label), "%.0f%%", (cat.second / total) * 100);
        float labelWidth = fOffscreenView->StringWidth(label);
        fOffscreenView->DrawString(label, BPoint(labelX - labelWidth/2, labelY));
        
        startAngle += angle;
        colorIndex++;
    }
    
    // Draw title
    fOffscreenView->SetHighColor(0, 0, 0);
    fOffscreenView->SetFontSize(11);
    fOffscreenView->DrawString("Category Distribution", BPoint(area.left + 5, area.top + 15));
}

void BenchmarkGraphView::DrawPerformanceMeter(BRect area)
{
    // Draw background
    fOffscreenView->SetHighColor(245, 245, 245);
    fOffscreenView->FillRect(area);
    
    // Draw border
    fOffscreenView->SetHighColor(180, 180, 180);
    fOffscreenView->StrokeRect(area);
    
    // Calculate overall score
    float totalScore = 0;
    if (!fResults.empty()) {
        for (const auto& result : fResults) {
            totalScore += result.score;
        }
        totalScore /= fResults.size();
    }
    
    // Draw speedometer-style gauge - properly centered
    BPoint center((area.left + area.right) / 2, (area.top + area.bottom) / 2 + 10);
    float radius = std::min(area.Width(), area.Height()) / 2 - 25;
    
    // Draw arc background
    fOffscreenView->SetHighColor(220, 220, 220);
    fOffscreenView->SetPenSize(15);
    fOffscreenView->StrokeArc(center, radius, radius, 45, 270);
    
    // Draw colored arc based on score
    rgb_color arcColor = GetScoreGradientColor(totalScore);
    fOffscreenView->SetHighColor(arcColor);
    float scoreAngle = 45 + (270 * (totalScore / 100.0f));
    fOffscreenView->StrokeArc(center, radius, radius, 45, scoreAngle - 45);
    fOffscreenView->SetPenSize(1);
    
    // Draw tick marks
    fOffscreenView->SetHighColor(100, 100, 100);
    for (int i = 0; i <= 10; i++) {
        float angle = 45 + (270 * i / 10.0f);
        float radAngle = angle * M_PI / 180.0f;
        float x1 = center.x + cosf(radAngle) * (radius - 20);
        float y1 = center.y + sinf(radAngle) * (radius - 20);
        float x2 = center.x + cosf(radAngle) * (radius - 10);
        float y2 = center.y + sinf(radAngle) * (radius - 10);
        fOffscreenView->StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
    }
    
    // Draw needle with triangle shape (more professional)
    float needleAngle = 45 + (270 * (totalScore / 100.0f));
    float radNeedle = needleAngle * M_PI / 180.0f;
    float needleX = center.x + cosf(radNeedle) * (radius - 25);
    float needleY = center.y + sinf(radNeedle) * (radius - 25);
    
    // Create needle as triangle using BPolygon (Haiku native)
    float perpAngle = radNeedle + M_PI / 2;
    BPoint needlePoints[3];
    needlePoints[0] = BPoint(needleX, needleY); // Tip
    needlePoints[1] = BPoint(center.x + cosf(perpAngle) * 5, 
                             center.y + sinf(perpAngle) * 5);
    needlePoints[2] = BPoint(center.x - cosf(perpAngle) * 5, 
                             center.y - sinf(perpAngle) * 5);
    
    // Draw needle shadow
    fOffscreenView->SetHighColor(0, 0, 0, 100);
    BPoint shadowPoints[3];
    for (int i = 0; i < 3; i++) {
        shadowPoints[i] = needlePoints[i];
        shadowPoints[i].x += 2;
        shadowPoints[i].y += 2;
    }
    fOffscreenView->FillPolygon(shadowPoints, 3);
    
    // Draw needle
    fOffscreenView->SetHighColor(220, 0, 0);
    fOffscreenView->FillPolygon(needlePoints, 3);
    fOffscreenView->SetHighColor(255, 50, 50);
    fOffscreenView->StrokePolygon(needlePoints, 3);
    
    // Draw center circle
    fOffscreenView->SetHighColor(50, 50, 50);
    fOffscreenView->FillEllipse(center, 8, 8);
    
    // Draw score text centered in the gauge
    char scoreText[64];
    snprintf(scoreText, sizeof(scoreText), "%.1f", totalScore);
    fOffscreenView->SetHighColor(0, 0, 0);
    fOffscreenView->SetFontSize(24);
    float textWidth = fOffscreenView->StringWidth(scoreText);
    font_height fh;
    fOffscreenView->GetFontHeight(&fh);
    float textHeight = fh.ascent + fh.descent;
    fOffscreenView->DrawString(scoreText, BPoint(center.x - textWidth/2, center.y + textHeight/2));
    
    // Draw title
    fOffscreenView->SetFontSize(11);
    fOffscreenView->DrawString("Overall Performance", BPoint(area.left + 5, area.top + 15));
}

void BenchmarkGraphView::DrawLiveStats(BRect area)
{
    // Draw background
    fOffscreenView->SetHighColor(245, 245, 245);
    fOffscreenView->FillRect(area);
    
    // Draw border
    fOffscreenView->SetHighColor(180, 180, 180);
    fOffscreenView->StrokeRect(area);
    
    // Draw title
    fOffscreenView->SetHighColor(0, 0, 0);
    fOffscreenView->SetFontSize(11);
    fOffscreenView->DrawString("Live Statistics", BPoint(area.left + 5, area.top + 15));
    
    // Draw stats
    float y = area.top + 35;
    fOffscreenView->SetFontSize(10);
    
    if (!fResults.empty()) {
        // Tests completed
        char stat[128];
        snprintf(stat, sizeof(stat), "Tests Completed: %zu", fResults.size());
        fOffscreenView->DrawString(stat, BPoint(area.left + 10, y));
        y += 20;
        
        // Best score
        float bestScore = 0;
        std::string bestTest;
        for (const auto& result : fResults) {
            if (result.score > bestScore) {
                bestScore = result.score;
                bestTest = result.name;
            }
        }
        
        snprintf(stat, sizeof(stat), "Best Score: %.1f%%", bestScore);
        fOffscreenView->SetHighColor(0, 150, 0);
        fOffscreenView->DrawString(stat, BPoint(area.left + 10, y));
        y += 20;
        
        // Current status
        fOffscreenView->SetHighColor(0, 0, 0);
        if (fProgress > 0 && fProgress < 100) {
            fOffscreenView->SetHighColor(0, 100, 200);
            snprintf(stat, sizeof(stat), "Status: Testing... %.0f%%", fProgress);
        } else if (fProgress >= 100) {
            fOffscreenView->SetHighColor(0, 150, 0);
            snprintf(stat, sizeof(stat), "Status: Complete");
        } else {
            snprintf(stat, sizeof(stat), "Status: Ready");
        }
        fOffscreenView->DrawString(stat, BPoint(area.left + 10, y));
    } else {
        fOffscreenView->DrawString("No tests run yet", BPoint(area.left + 10, y));
    }
}

void BenchmarkGraphView::DrawModernProgressBar(BRect rect, float percentage)
{
    // Draw background
    fOffscreenView->SetHighColor(200, 200, 200);
    fOffscreenView->FillRoundRect(rect, 5, 5);
    
    // Draw progress fill
    if (percentage > 0) {
        BRect fillRect = rect;
        fillRect.right = fillRect.left + (rect.Width() * percentage / 100.0f);
        
        // Gradient color based on progress
        rgb_color fillColor;
        if (percentage < 33) {
            fillColor = (rgb_color){255, 100, 100, 255}; // Red
        } else if (percentage < 66) {
            fillColor = (rgb_color){255, 200, 0, 255}; // Yellow
        } else {
            fillColor = (rgb_color){0, 200, 0, 255}; // Green
        }
        
        fOffscreenView->SetHighColor(fillColor);
        fOffscreenView->FillRoundRect(fillRect, 5, 5);
        
        // Draw animated stripes
        int stripeOffset = ((int)(system_time() / 50000)) % 20;
        fOffscreenView->SetHighColor(255, 255, 255, 50);
        for (int x = fillRect.left - 20; x < fillRect.right; x += 20) {
            BRect stripe(x + stripeOffset, fillRect.top, 
                        x + stripeOffset + 10, fillRect.bottom);
            if (stripe.left < fillRect.right && stripe.right > fillRect.left) {
                if (stripe.left < fillRect.left) stripe.left = fillRect.left;
                if (stripe.right > fillRect.right) stripe.right = fillRect.right;
                fOffscreenView->FillRect(stripe);
            }
        }
    }
    
    // Draw border
    fOffscreenView->SetHighColor(100, 100, 100);
    fOffscreenView->StrokeRoundRect(rect, 5, 5);
    
    // Draw percentage text
    char text[32];
    snprintf(text, sizeof(text), "%.0f%%", percentage);
    fOffscreenView->SetHighColor(0, 0, 0);
    fOffscreenView->SetFontSize(10);
    float textWidth = fOffscreenView->StringWidth(text);
    fOffscreenView->DrawString(text, 
        BPoint(rect.left + (rect.Width() - textWidth) / 2, rect.top - 3));
}

rgb_color BenchmarkGraphView::GetScoreGradientColor(float score)
{
    // Smooth gradient from red to yellow to green
    rgb_color color;
    if (score < 50) {
        // Red to Yellow
        float t = score / 50.0f;
        color.red = 255;
        color.green = (uint8)(255 * t);
        color.blue = 0;
    } else {
        // Yellow to Green
        float t = (score - 50) / 50.0f;
        color.red = (uint8)(255 * (1 - t));
        color.green = 200 + (uint8)(55 * t);
        color.blue = 0;
    }
    color.alpha = 255;
    return color;
}

rgb_color BenchmarkGraphView::GetCategoryColor(int index)
{
    rgb_color colors[] = {
        {100, 150, 255, 255}, // Blue
        {255, 150, 100, 255}, // Orange
        {150, 255, 100, 255}, // Green
        {255, 100, 150, 255}, // Pink
        {150, 100, 255, 255}, // Purple
        {255, 255, 100, 255}  // Yellow
    };
    return colors[index % 6];
}

// BenchmarkWindow Implementation
BenchmarkWindow::BenchmarkWindow(BRect frame)
    : BWindow(frame, "HaikuMix Performance Benchmark", B_TITLED_WINDOW,
              B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
    , fBenchmark(nullptr)
    , fBenchmarkThread(-1)
    , fRunning(false)
{
    InitUI();
    
    // Center window
    CenterOnScreen();
}

BenchmarkWindow::~BenchmarkWindow()
{
    if (fRunning && fBenchmarkThread >= 0) {
        // Stop benchmark thread
        kill_thread(fBenchmarkThread);
    }
    delete fBenchmark;
}

void BenchmarkWindow::InitUI()
{
    // Create main layout
    BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 0);
    SetLayout(layout);
    
    // Create tab view
    fTabView = new BTabView("tabs");
    
    // Overview tab with graphs
    BRect tabRect = fTabView->Bounds();
    tabRect.InsetBy(5, 5);
    fGraphView = new BenchmarkGraphView(tabRect, "graph");
    BTab* overviewTab = new BTab();
    fTabView->AddTab(fGraphView, overviewTab);
    overviewTab->SetLabel("Overview");
    
    // Results list tab
    fResultsList = new BListView("results");
    BScrollView* scrollView = new BScrollView("scroll", fResultsList,
        B_FOLLOW_ALL, 0, true, true);
    BTab* detailsTab = new BTab();
    fTabView->AddTab(scrollView, detailsTab);
    detailsTab->SetLabel("Detailed Results");
    
    // Control buttons
    fRunAllButton = new BButton("Run All Tests", new BMessage(MSG_RUN_ALL_TESTS));
    fStopButton = new BButton("Stop", new BMessage(MSG_STOP_TEST));
    fExportButton = new BButton("Export Results", new BMessage(MSG_EXPORT_RESULTS));
    fHistoryButton = new BButton("History", new BMessage(MSG_SHOW_HISTORY));
    
    fStopButton->SetEnabled(false);
    fExportButton->SetEnabled(false);
    
    // Category test buttons
    fAudioButton = new BButton("Audio Tests", new BMessage(MSG_RUN_AUDIO_TEST));
    f3DButton = new BButton("3D Tests", new BMessage(MSG_RUN_3D_TEST));
    fMemoryButton = new BButton("Memory Tests", new BMessage(MSG_RUN_MEMORY_TEST));
    fSystemButton = new BButton("System Tests", new BMessage(MSG_RUN_SYSTEM_TEST));
    
    // System info display
    system_info sysInfo;
    get_system_info(&sysInfo);
    
    char infoText[512];
    snprintf(infoText, sizeof(infoText),
        "System: %d cores | RAM: %ldMB total, %ldMB used | Kernel: %s",
        sysInfo.cpu_count,
        sysInfo.max_pages * B_PAGE_SIZE / (1024 * 1024),
        sysInfo.used_pages * B_PAGE_SIZE / (1024 * 1024),
        sysInfo.kernel_build_date);
    
    fSystemInfoView = new BStringView("sysinfo", infoText);
    fSystemInfoView->SetFontSize(10);
    
    // Progress bar
    fProgressBar = new BStatusBar("progress", "Ready");
    fProgressBar->SetMaxValue(100.0f);
    
    // Status text
    fStatusText = new BStringView("status", "Click 'Run All Tests' to begin benchmark");
    
    // Build layout
    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .SetInsets(B_USE_WINDOW_SPACING)
        .Add(fTabView)
        .AddGroup(B_HORIZONTAL)
            .Add(fRunAllButton)
            .Add(fAudioButton)
            .Add(f3DButton)
            .Add(fMemoryButton)
            .Add(fSystemButton)
            .Add(fStopButton)
            .Add(fExportButton)
            .Add(fHistoryButton)
            .AddGlue()
        .End()
        .Add(fSystemInfoView)
        .Add(fProgressBar)
        .Add(fStatusText)
    .End();
}

void BenchmarkWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_RUN_ALL_TESTS:
            RunBenchmark(MSG_RUN_ALL_TESTS);
            break;
            
        case MSG_RUN_AUDIO_TEST:
            RunCategoryBenchmark("audio");
            break;
            
        case MSG_RUN_3D_TEST:
            RunCategoryBenchmark("3d");
            break;
            
        case MSG_RUN_MEMORY_TEST:
            RunCategoryBenchmark("memory");
            break;
            
        case MSG_RUN_SYSTEM_TEST:
            RunCategoryBenchmark("system");
            break;
            
        case MSG_STOP_TEST:
            if (fRunning && fBenchmarkThread >= 0) {
                kill_thread(fBenchmarkThread);
                fRunning = false;
                fStatusText->SetText("Benchmark stopped");
                fStopButton->SetEnabled(false);
                fRunAllButton->SetEnabled(true);
            }
            break;
            
        case MSG_EXPORT_RESULTS:
            ExportResults();
            break;
            
        case MSG_CLEAR_RESULTS:
            ClearResults();
            break;
            
        case MSG_SHOW_HISTORY:
            ShowBenchmarkHistory();
            break;
            
        case MSG_TEST_UPDATE:
        {
            float progress;
            if (message->FindFloat("progress", &progress) == B_OK) {
                fProgressBar->Update(progress - fProgressBar->CurrentValue());
                fGraphView->UpdateProgress(progress);
            }
            
            const char* status;
            if (message->FindString("status", &status) == B_OK) {
                fStatusText->SetText(status);
            }
            break;
        }
        
        case MSG_TEST_COMPLETE:
        {
            // First update all results and views
            UpdateResults();
            
            // Save to history
            SaveBenchmarkHistory();
            
            // Update state
            fRunning = false;
            fStopButton->SetEnabled(false);
            fRunAllButton->SetEnabled(true);
            fAudioButton->SetEnabled(true);
            f3DButton->SetEnabled(true);
            fMemoryButton->SetEnabled(true);
            fSystemButton->SetEnabled(true);
            fExportButton->SetEnabled(true);
            fHistoryButton->SetEnabled(true);
            fStatusText->SetText("Benchmark complete! Check 'Detailed Results' tab for full report.");
            
            // Force graph refresh
            fGraphView->Invalidate();
            
            // Show alert with summary
            char summary[256];
            float score = fBenchmark ? fBenchmark->GetTotalScore() : 0.0f;
            const char* rating;
            if (score >= 90) rating = "EXCELLENT";
            else if (score >= 75) rating = "VERY GOOD";
            else if (score >= 60) rating = "GOOD";
            else if (score >= 45) rating = "FAIR";
            else rating = "NEEDS IMPROVEMENT";
            
            snprintf(summary, sizeof(summary), 
                "Benchmark Complete!\n\nOverall Score: %.1f/100\nRating: %s\n\nCheck the 'Detailed Results' tab for full breakdown.",
                score, rating);
            
            BAlert* alert = new BAlert("Complete", summary, "OK", nullptr, nullptr,
                B_WIDTH_AS_USUAL, B_INFO_ALERT);
            alert->Go();
            break;
        }
            
        default:
            BWindow::MessageReceived(message);
    }
}

void BenchmarkWindow::RunBenchmark(uint32 testType)
{
    if (fRunning) return;
    
    fRunning = true;
    fRunAllButton->SetEnabled(false);
    fAudioButton->SetEnabled(false);
    f3DButton->SetEnabled(false);
    fMemoryButton->SetEnabled(false);
    fSystemButton->SetEnabled(false);
    fStopButton->SetEnabled(true);
    fExportButton->SetEnabled(false);
    
    // Clear previous results
    ClearResults();
    
    // Create benchmark if needed
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    
    // Start benchmark in separate thread
    fStatusText->SetText("Running benchmark tests...");
    fProgressBar->Reset();
    fProgressBar->SetMaxValue(100.0f);
    
    // Start benchmark in separate thread
    fBenchmarkThread = spawn_thread(BenchmarkThreadEntry, "benchmark_thread", 
                                   B_NORMAL_PRIORITY, this);
    if (fBenchmarkThread >= 0) {
        resume_thread(fBenchmarkThread);
    } else {
        // Fallback to immediate completion if thread fails
        PostMessage(new BMessage(MSG_TEST_COMPLETE), this);
    }
}

void BenchmarkWindow::UpdateResults()
{
    if (!fBenchmark) return;
    
    // Get results from benchmark
    const auto& results = fBenchmark->GetResults();
    
    // Update graph view
    fGraphView->SetData(results);
    
    // Calculate category scores
    std::map<std::string, float> categoryScores;
    std::map<std::string, int> categoryCounts;
    
    for (const auto& result : results) {
        categoryScores[result.category] += result.score;
        categoryCounts[result.category]++;
    }
    
    for (auto& cat : categoryScores) {
        cat.second /= categoryCounts[cat.first];
    }
    
    fGraphView->SetCategoryData(categoryScores);
    
    // Update results list - clear and repopulate
    fResultsList->MakeEmpty();
    
    // Add header item
    fResultsList->AddItem(new BStringItem("=== BENCHMARK RESULTS ==="));
    fResultsList->AddItem(new BStringItem(""));
    
    // Group results by category
    std::map<std::string, std::vector<BenchmarkResult>> groupedResults;
    for (const auto& result : results) {
        groupedResults[result.category].push_back(result);
    }
    
    // Display results grouped by category
    for (const auto& group : groupedResults) {
        // Category header
        char categoryHeader[128];
        snprintf(categoryHeader, sizeof(categoryHeader), "[%s]", group.first.c_str());
        fResultsList->AddItem(new BStringItem(categoryHeader));
        
        // Results in this category
        for (const auto& result : group.second) {
            char item[512];
            // More detailed format with better alignment
            snprintf(item, sizeof(item), "  • %-40s: %8.2f %-10s (Score: %5.1f/100)",
                result.name.c_str(), result.value, result.unit.c_str(), result.score);
            fResultsList->AddItem(new BStringItem(item));
        }
        
        fResultsList->AddItem(new BStringItem("")); // Spacing between categories
    }
    
    // Add summary at the end
    float totalScore = 0;
    if (!results.empty()) {
        for (const auto& result : results) {
            totalScore += result.score;
        }
        totalScore /= results.size();
        
        fResultsList->AddItem(new BStringItem("=== SUMMARY ==="));
        char summary[256];
        snprintf(summary, sizeof(summary), "Total Tests: %zu", results.size());
        fResultsList->AddItem(new BStringItem(summary));
        snprintf(summary, sizeof(summary), "Overall Score: %.1f/100", totalScore);
        fResultsList->AddItem(new BStringItem(summary));
        
        // Performance rating
        const char* rating;
        if (totalScore >= 90) rating = "EXCELLENT";
        else if (totalScore >= 75) rating = "VERY GOOD";
        else if (totalScore >= 60) rating = "GOOD";
        else if (totalScore >= 45) rating = "FAIR";
        else rating = "NEEDS IMPROVEMENT";
        
        snprintf(summary, sizeof(summary), "Performance Rating: %s", rating);
        fResultsList->AddItem(new BStringItem(summary));
    }
}

void BenchmarkWindow::ExportResults()
{
    if (!fBenchmark) return;
    
    // Create export dialog
    BAlert* exportDialog = new BAlert("Export Format", 
        "Choose export format:",
        "TXT", "HTML", "CSV", 
        B_WIDTH_AS_USUAL, B_INFO_ALERT);
    
    int32 choice = exportDialog->Go();
    
    if (choice == 0) {
        ExportTXT();
    } else if (choice == 1) {
        ExportHTML();
    } else if (choice == 2) {
        ExportCSV();
    }
}

void BenchmarkWindow::ClearResults()
{
    fResultsList->MakeEmpty();
    fGraphView->SetData(std::vector<BenchmarkResult>());
    fGraphView->SetCategoryData(std::map<std::string, float>());
    fProgressBar->Reset();
}

bool BenchmarkWindow::QuitRequested()
{
    if (fRunning) {
        BAlert* alert = new BAlert("Quit",
            "Benchmark is running. Stop and quit?",
            "Cancel", "Stop & Quit", nullptr,
            B_WIDTH_AS_USUAL, B_WARNING_ALERT);
            
        if (alert->Go() == 1) {
            if (fBenchmarkThread >= 0) {
                kill_thread(fBenchmarkThread);
            }
            return true;
        }
        return false;
    }
    
    return true;
}

// BenchmarkDetailView Implementation
BenchmarkDetailView::BenchmarkDetailView(BRect frame, const char* name)
    : BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

BenchmarkDetailView::~BenchmarkDetailView()
{
}

void BenchmarkDetailView::Draw(BRect updateRect)
{
    BRect bounds = Bounds();
    
    // Draw speedometer for score
    DrawSpeedometer(bounds, fResult.score);
}

void BenchmarkDetailView::SetResult(const BenchmarkResult& result)
{
    fResult = result;
    Invalidate();
}

void BenchmarkDetailView::DrawSpeedometer(BRect bounds, float score)
{
    BPoint center(bounds.Width() / 2, bounds.Height() / 2);
    float radius = std::min(bounds.Width(), bounds.Height()) / 2 - 20;
    
    // Draw arc background
    SetHighColor(200, 200, 200);
    StrokeArc(center, radius, radius, 45, 270);
    
    // Draw colored arc based on score
    SetHighColor(GetScoreColor(score));
    SetPenSize(5.0);
    float angle = 45 + (270 * (score / 100.0f));
    StrokeArc(center, radius, radius, 45, angle - 45);
    SetPenSize(1.0);
    
    // Draw score text
    SetHighColor(0, 0, 0);
    SetFontSize(24);
    char scoreText[32];
    snprintf(scoreText, sizeof(scoreText), "%.1f", score);
    DrawString(scoreText, BPoint(center.x - 20, center.y));
}

rgb_color BenchmarkDetailView::GetScoreColor(float score)
{
    if (score >= 90) return (rgb_color){0, 200, 0, 255};      // Green
    if (score >= 75) return (rgb_color){0, 150, 200, 255};    // Blue
    if (score >= 50) return (rgb_color){255, 200, 0, 255};    // Yellow
    return (rgb_color){255, 100, 100, 255};                    // Red
}

// Static thread entry point
int32 BenchmarkWindow::BenchmarkThreadEntry(void* data)
{
    BenchmarkWindow* window = static_cast<BenchmarkWindow*>(data);
    window->RunBenchmarkTests();
    return B_OK;
}

void BenchmarkWindow::RunBenchmarkTests()
{
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    
    // Clear previous results
    fBenchmark->ClearResults();
    
    // Update progress and status
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 10.0f);
    progressMsg->AddString("status", "Starting audio engine test...");
    PostMessage(progressMsg, this);
    
    // Run comprehensive audio tests
    TestAudioEngineSimple();
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 25.0f);
    progressMsg->AddString("status", "Audio engine test complete, testing latency...");
    PostMessage(progressMsg, this);
    
    TestAudioLatency();
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 35.0f);
    progressMsg->AddString("status", "Latency test complete, testing sine generation...");
    PostMessage(progressMsg, this);
    
    TestSineGeneration();
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 45.0f);
    progressMsg->AddString("status", "Sine test complete, testing DSP throughput...");
    PostMessage(progressMsg, this);
    
    TestBufferProcessing();
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 50.0f);
    progressMsg->AddString("status", "All audio tests complete, starting memory test...");
    PostMessage(progressMsg, this);
    
    snooze(500000); // Brief pause between tests
    
    // Run memory bandwidth test
    TestMemoryBandwidth();
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 75.0f);
    progressMsg->AddString("status", "Memory test complete, starting CPU scaling test...");
    PostMessage(progressMsg, this);
    
    snooze(500000); // Brief pause between tests
    
    // Run CPU scaling test
    TestCPUScaling();
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 100.0f);
    progressMsg->AddString("status", "All tests complete!");
    PostMessage(progressMsg, this);
    
    // Calculate final score
    if (fBenchmark) {
        float totalScore = 0.0f;
        const auto& results = fBenchmark->GetResults();
        if (!results.empty()) {
            for (const auto& result : results) {
                totalScore += result.score;
            }
            totalScore /= results.size();
            // Update benchmark's total score
            fBenchmark->SetTotalScore(totalScore);
        }
    }
    
    // Signal completion
    PostMessage(new BMessage(MSG_TEST_COMPLETE), this);
}

void BenchmarkWindow::TestAudioEngineSimple()
{
    // Comprehensive audio processing benchmark
    const int bufferSize = 512;
    const int iterations = 2000; // More iterations for better averaging
    float* buffer = new float[bufferSize * 2]; // Stereo
    
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddString("status", "Initializing 512-sample stereo audio buffers...");
    PostMessage(progressMsg, this);
    
    snooze(300000); // Brief pause for user to read
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 15.0f);
    progressMsg->AddString("status", "Warming up CPU cache (100 cycles)...");
    PostMessage(progressMsg, this);
    
    // Warm up to stabilize performance
    for (int warmup = 0; warmup < 100; warmup++) {
        for (int j = 0; j < bufferSize * 2; j++) {
            buffer[j] = sinf(j * 0.01f) * 0.5f;
        }
    }
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 25.0f);
    progressMsg->AddString("status", "Running DSP chain: filter→gain→reverb (2000 iterations)...");
    PostMessage(progressMsg, this);
    
    bigtime_t startTime = system_time();
    
    for (int i = 0; i < iterations; i++) {
        // Realistic audio callback processing
        for (int j = 0; j < bufferSize * 2; j++) {
            // Generate sine wave
            buffer[j] = sinf(j * 0.01f + i * 0.001f) * 0.7f;
            
            // Realistic DSP chain:
            // 1. High-pass filter (DC removal)
            static float hp_z1 = 0.0f;
            float hp_out = buffer[j] - hp_z1 * 0.995f;
            hp_z1 = buffer[j];
            
            // 2. Gain adjustment
            hp_out *= 0.8f;
            
            // 3. Simple reverb (delay + feedback)
            static float delay_buffer[128] = {0};
            static int delay_idx = 0;
            float delayed = delay_buffer[delay_idx];
            delay_buffer[delay_idx] = hp_out + delayed * 0.3f;
            delay_idx = (delay_idx + 1) % 128;
            
            buffer[j] = hp_out + delayed * 0.2f;
        }
        
        // Update progress every 200 iterations with detailed info
        if (i % 200 == 0) {
            float progress = 25.0f + (i / (float)iterations) * 65.0f;
            progressMsg = new BMessage(MSG_TEST_UPDATE);
            progressMsg->AddFloat("progress", progress);
            
            char statusText[256];
            float percentComplete = (i / (float)iterations) * 100.0f;
            snprintf(statusText, sizeof(statusText), 
                "Processing audio samples... %.1f%% (%d/%d callbacks)", 
                percentComplete, i, iterations);
            progressMsg->AddString("status", statusText);
            PostMessage(progressMsg, this);
        }
    }
    
    bigtime_t endTime = system_time();
    delete[] buffer;
    
    // Calculate detailed results
    float duration = (endTime - startTime) / 1000.0f; // ms
    float avgCallbackTime = duration / iterations;
    float bufferTime = (bufferSize / 44100.0f) * 1000.0f; // ms (~11.6ms @ 512 samples)
    float cpuUsage = (avgCallbackTime / bufferTime) * 100.0f;
    float efficiency = std::max(0.0f, std::min(100.0f, 100.0f - cpuUsage));
    
    // Calculate additional metrics
    float samplesPerSec = (bufferSize * 2 * iterations * 1000.0f) / duration; // samples/sec
    float throughputMB = (samplesPerSec * sizeof(float)) / (1024.0f * 1024.0f); // MB/s
    int maxTracks = efficiency > 10.0f ? (int)(100.0f / cpuUsage) : 1;
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 95.0f);
    char detailStatus[256];
    snprintf(detailStatus, sizeof(detailStatus), 
        "Results: %.3fms/callback, %.1f%% CPU, ~%d max tracks", 
        avgCallbackTime, cpuUsage, maxTracks);
    progressMsg->AddString("status", detailStatus);
    PostMessage(progressMsg, this);
    
    // Create comprehensive result with detailed info
    BenchmarkResult result;
    result.name = "Audio Engine Performance";
    result.category = "Audio Processing";
    result.value = avgCallbackTime;
    result.unit = "ms/callback";
    result.duration = duration;
    result.score = efficiency;
    
    // Add detailed info to result name for display
    char detailedName[256];
    snprintf(detailedName, sizeof(detailedName), 
        "Audio Engine (%.1fMB/s, %d max tracks)", throughputMB, maxTracks);
    result.name = detailedName;
    
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    fBenchmark->AddResult(result);
}

void BenchmarkWindow::TestMemoryBandwidth()
{
    // Memory bandwidth benchmark
    const size_t bufferSize = 8 * 1024 * 1024; // 8 MB
    const int iterations = 50;
    char* src = new char[bufferSize];
    char* dst = new char[bufferSize];
    
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 52.0f);
    progressMsg->AddString("status", "Allocating 8MB memory buffers...");
    PostMessage(progressMsg, this);
    
    // Initialize source buffer
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 55.0f);
    progressMsg->AddString("status", "Initializing memory patterns...");
    PostMessage(progressMsg, this);
    
    for (size_t i = 0; i < bufferSize; i++) {
        src[i] = i & 0xFF;
    }
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 60.0f);
    progressMsg->AddString("status", "Testing memory copy performance (50 iterations)...");
    PostMessage(progressMsg, this);
    
    bigtime_t startTime = system_time();
    
    for (int i = 0; i < iterations; i++) {
        memcpy(dst, src, bufferSize);
        
        // Update progress every 10 iterations
        if (i % 10 == 0) {
            float progress = 60.0f + (i / (float)iterations) * 30.0f;
            progressMsg = new BMessage(MSG_TEST_UPDATE);
            progressMsg->AddFloat("progress", progress);
            
            char statusText[256];
            snprintf(statusText, sizeof(statusText), 
                "Memory copy test: %d/%d iterations (%.1f%%)", 
                i, iterations, (i / (float)iterations) * 100.0f);
            progressMsg->AddString("status", statusText);
            PostMessage(progressMsg, this);
        }
    }
    
    bigtime_t endTime = system_time();
    
    delete[] src;
    delete[] dst;
    
    // Calculate results
    float duration = (endTime - startTime) / 1000000.0f; // Convert to seconds
    float totalDataMB = (bufferSize * iterations * 2) / (1024.0f * 1024.0f); // *2 for read+write
    float bandwidth = totalDataMB / duration; // MB/s
    float score = std::min(100.0f, (bandwidth / 2000.0f) * 100.0f); // 2000 MB/s = 100 score
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 95.0f);
    char detailStatus[256];
    snprintf(detailStatus, sizeof(detailStatus), 
        "Memory: %.1f MB/s bandwidth, %.2f seconds total", 
        bandwidth, duration);
    progressMsg->AddString("status", detailStatus);
    PostMessage(progressMsg, this);
    
    // Create result
    BenchmarkResult result;
    char detailedName[256];
    snprintf(detailedName, sizeof(detailedName), 
        "Memory Bandwidth (%.1fMB total, %.2fs)", totalDataMB, duration);
    result.name = detailedName;
    result.category = "Memory";
    result.value = bandwidth;
    result.unit = "MB/s";
    result.duration = duration * 1000.0f; // Convert back to ms
    result.score = score;
    
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    fBenchmark->AddResult(result);
}

void BenchmarkWindow::TestCPUScaling()
{
    // CPU scaling efficiency test
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 77.0f);
    progressMsg->AddString("status", "Detecting CPU configuration...");
    PostMessage(progressMsg, this);
    
    // Get system info
    system_info sysInfo;
    get_system_info(&sysInfo);
    int numCores = sysInfo.cpu_count;
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 78.0f);
    char cpuInfo[256];
    snprintf(cpuInfo, sizeof(cpuInfo), 
        "Found %d CPU cores, testing single-thread performance...", numCores);
    progressMsg->AddString("status", cpuInfo);
    PostMessage(progressMsg, this);
    
    snooze(500000); // Let user see CPU info
    
    // Test single-threaded performance
    const int workSize = 10000000; // 10M operations
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 80.0f);
    progressMsg->AddString("status", "Running single-thread benchmark (10M operations)...");
    PostMessage(progressMsg, this);
    
    bigtime_t singleStart = system_time();
    volatile float singleResult = 0.0f;
    
    // Single thread work
    for (int i = 0; i < workSize; i++) {
        singleResult += sinf(i * 0.0001f) * cosf(i * 0.0002f);
    }
    
    bigtime_t singleEnd = system_time();
    float singleTime = (singleEnd - singleStart) / 1000.0f; // ms
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 85.0f);
    char singleStatus[256];
    snprintf(singleStatus, sizeof(singleStatus), 
        "Single-thread: %.2fms, starting multi-thread test (%d threads)...", 
        singleTime, numCores);
    progressMsg->AddString("status", singleStatus);
    PostMessage(progressMsg, this);
    
    // Test multi-threaded performance
    thread_id* threads = new thread_id[numCores];
    CPUWorkData* workData = new CPUWorkData[numCores];
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 88.0f);
    progressMsg->AddString("status", "Spawning worker threads...");
    PostMessage(progressMsg, this);
    
    bigtime_t multiStart = system_time();
    
    // Spawn threads
    for (int core = 0; core < numCores; core++) {
        workData[core].workSize = workSize / numCores;
        workData[core].result = 0.0f;
        workData[core].threadId = core;
        
        char threadName[32];
        snprintf(threadName, sizeof(threadName), "cpu_test_%d", core);
        
        threads[core] = spawn_thread(CPUWorkerThread, threadName, 
                                    B_NORMAL_PRIORITY, &workData[core]);
        if (threads[core] >= 0) {
            resume_thread(threads[core]);
        }
    }
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 90.0f);
    char threadStatus[256];
    snprintf(threadStatus, sizeof(threadStatus), 
        "Running %d threads in parallel...", numCores);
    progressMsg->AddString("status", threadStatus);
    PostMessage(progressMsg, this);
    
    // Wait for all threads
    for (int core = 0; core < numCores; core++) {
        status_t status;
        wait_for_thread(threads[core], &status);
    }
    
    bigtime_t multiEnd = system_time();
    float multiTime = (multiEnd - multiStart) / 1000.0f; // ms
    
    delete[] threads;
    delete[] workData;
    
    // Calculate results
    float speedup = singleTime / multiTime;
    float efficiency = (speedup / numCores) * 100.0f;
    float score = std::min(100.0f, efficiency);
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 98.0f);
    char detailStatus[256];
    snprintf(detailStatus, sizeof(detailStatus), 
        "CPU: %.1fx speedup, %.1f%% efficiency (%d cores)", 
        speedup, efficiency, numCores);
    progressMsg->AddString("status", detailStatus);
    PostMessage(progressMsg, this);
    
    // Create result
    BenchmarkResult result;
    char detailedName[256];
    snprintf(detailedName, sizeof(detailedName), 
        "CPU Scaling (%d cores, %.1fx speedup)", numCores, speedup);
    result.name = detailedName;
    result.category = "CPU";
    result.value = efficiency;
    result.unit = "% efficiency";
    result.duration = multiTime;
    result.score = score;
    
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    fBenchmark->AddResult(result);
}

// Static thread worker function
int32 BenchmarkWindow::CPUWorkerThread(void* data)
{
    CPUWorkData* workData = static_cast<CPUWorkData*>(data);
    
    // Each thread does its portion of work
    for (int i = 0; i < workData->workSize; i++) {
        int index = i + (workData->threadId * workData->workSize);
        workData->result += sinf(index * 0.0001f) * cosf(index * 0.0002f);
    }
    
    return B_OK;
}

void BenchmarkWindow::TestAudioLatency()
{
    // Test different buffer sizes for latency
    int bufferSizes[] = {64, 128, 256, 512, 1024, 2048};
    const int numSizes = 6;
    const float sampleRate = 44100.0f;
    
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddString("status", "Testing audio latency with different buffer sizes...");
    PostMessage(progressMsg, this);
    
    float bestLatency = 1000.0f;
    int bestBufferSize = 512;
    float totalScore = 0.0f;
    
    for (int i = 0; i < numSizes; i++) {
        int bufferSize = bufferSizes[i];
        float latency = (bufferSize / sampleRate) * 1000.0f; // ms
        
        // Test processing time for this buffer size
        float* buffer = new float[bufferSize * 2]; // Stereo
        
        bigtime_t startTime = system_time();
        for (int j = 0; j < 1000; j++) {
            // Simulate audio callback
            for (int k = 0; k < bufferSize * 2; k++) {
                buffer[k] = sinf(k * 0.01f) * 0.5f;
            }
        }
        bigtime_t endTime = system_time();
        delete[] buffer;
        
        float processingTime = (endTime - startTime) / 1000000.0f; // seconds
        float timePerBuffer = processingTime * 1000.0f; // ms
        
        // Calculate if this buffer size is viable
        bool viable = (timePerBuffer < latency * 0.8f); // Need 20% headroom
        
        if (viable && latency < bestLatency) {
            bestLatency = latency;
            bestBufferSize = bufferSize;
        }
        
        // Score based on latency (lower is better)
        float score = 0;
        if (latency <= 3.0f) score = 100.0f;
        else if (latency <= 6.0f) score = 90.0f;
        else if (latency <= 12.0f) score = 75.0f;
        else if (latency <= 24.0f) score = 50.0f;
        else score = 25.0f;
        
        if (viable) totalScore += score;
        
        // Update progress
        char statusText[256];
        snprintf(statusText, sizeof(statusText), 
            "Buffer %d samples: %.2fms latency (%s)", 
            bufferSize, latency, viable ? "OK" : "Too slow");
        progressMsg = new BMessage(MSG_TEST_UPDATE);
        progressMsg->AddString("status", statusText);
        PostMessage(progressMsg, this);
    }
    
    // Create result
    BenchmarkResult result;
    char detailedName[256];
    snprintf(detailedName, sizeof(detailedName), 
        "Audio Latency (Best: %d samples @ %.2fms)", bestBufferSize, bestLatency);
    result.name = detailedName;
    result.category = "Audio Processing";
    result.value = bestLatency;
    result.unit = "ms";
    result.duration = 0;
    result.score = totalScore / numSizes;
    
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    fBenchmark->AddResult(result);
}

void BenchmarkWindow::TestSineGeneration()
{
    // Test sine generation speed
    const int numSamples = 1000000;
    float* buffer = new float[numSamples];
    
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddString("status", "Testing sine wave generation performance...");
    PostMessage(progressMsg, this);
    
    // Test standard sinf()
    float phase = 0.0f;
    const float phaseInc = 2.0f * M_PI * 440.0f / 44100.0f;
    
    bigtime_t standardStart = system_time();
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = sinf(phase);
        phase += phaseInc;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    }
    bigtime_t standardEnd = system_time();
    float standardTime = (standardEnd - standardStart) / 1000.0f; // ms
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    char statusText[256];
    snprintf(statusText, sizeof(statusText), 
        "Standard sinf(): %.2fms for 1M samples", standardTime);
    progressMsg->AddString("status", statusText);
    PostMessage(progressMsg, this);
    
    // Test lookup table method
    const int TABLE_SIZE = 4096;
    float* sineTable = new float[TABLE_SIZE];
    for (int i = 0; i < TABLE_SIZE; i++) {
        sineTable[i] = sinf((i / (float)TABLE_SIZE) * 2.0f * M_PI);
    }
    
    phase = 0.0f;
    bigtime_t lookupStart = system_time();
    for (int i = 0; i < numSamples; i++) {
        int index = (int)(phase * TABLE_SIZE / (2.0f * M_PI)) % TABLE_SIZE;
        buffer[i] = sineTable[index];
        phase += phaseInc;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    }
    bigtime_t lookupEnd = system_time();
    float lookupTime = (lookupEnd - lookupStart) / 1000.0f; // ms
    
    delete[] sineTable;
    delete[] buffer;
    
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    snprintf(statusText, sizeof(statusText), 
        "Lookup table: %.2fms (%.1fx speedup)", 
        lookupTime, standardTime / lookupTime);
    progressMsg->AddString("status", statusText);
    PostMessage(progressMsg, this);
    
    // Create result
    float speedup = standardTime / lookupTime;
    BenchmarkResult result;
    char detailedName[256];
    snprintf(detailedName, sizeof(detailedName), 
        "Sine Generation (%.1fx speedup with lookup)", speedup);
    result.name = detailedName;
    result.category = "Audio Processing";
    result.value = speedup;
    result.unit = "x faster";
    result.duration = lookupTime;
    result.score = std::min(100.0f, speedup * 25.0f); // 4x speedup = 100
    
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    fBenchmark->AddResult(result);
}

void BenchmarkWindow::TestBufferProcessing()
{
    // Test DSP throughput with complex filters
    const int bufferSize = 512;
    const int channels = 2;
    const int iterations = 5000;
    float* input = new float[bufferSize * channels];
    float* output = new float[bufferSize * channels];
    
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddString("status", "Testing DSP throughput with filters...");
    PostMessage(progressMsg, this);
    
    // Initialize test signal
    for (int i = 0; i < bufferSize * channels; i++) {
        input[i] = ((rand() % 1000) / 500.0f) - 1.0f; // Random -1 to 1
    }
    
    // Biquad filter coefficients (typical EQ)
    float a1 = -1.979f, a2 = 0.9802f;
    float b0 = 0.0001f, b1 = 0.0002f, b2 = 0.0001f;
    float z1[2] = {0}, z2[2] = {0}; // State for stereo
    
    bigtime_t startTime = system_time();
    
    for (int iter = 0; iter < iterations; iter++) {
        // Process each channel
        for (int ch = 0; ch < channels; ch++) {
            for (int i = 0; i < bufferSize; i++) {
                int idx = i * channels + ch;
                
                // Biquad filter
                float in = input[idx];
                float out = b0 * in + z1[ch];
                z1[ch] = b1 * in - a1 * out + z2[ch];
                z2[ch] = b2 * in - a2 * out;
                
                // Compressor simulation
                float absOut = fabsf(out);
                float gain = 1.0f;
                if (absOut > 0.7f) {
                    gain = 0.7f / absOut; // Simple limiting
                }
                
                output[idx] = out * gain;
            }
        }
        
        // Update progress periodically
        if (iter % 500 == 0) {
            char statusText[256];
            snprintf(statusText, sizeof(statusText), 
                "Processing DSP filters... %d/%d iterations", iter, iterations);
            progressMsg = new BMessage(MSG_TEST_UPDATE);
            progressMsg->AddString("status", statusText);
            PostMessage(progressMsg, this);
        }
    }
    
    bigtime_t endTime = system_time();
    delete[] input;
    delete[] output;
    
    float duration = (endTime - startTime) / 1000.0f; // ms
    float samplesPerSec = (bufferSize * channels * iterations * 1000.0f) / duration;
    float throughputMB = (samplesPerSec * sizeof(float)) / (1024.0f * 1024.0f);
    
    // Create result
    BenchmarkResult result;
    char detailedName[256];
    snprintf(detailedName, sizeof(detailedName), 
        "DSP Processing (%.1f MB/s throughput)", throughputMB);
    result.name = detailedName;
    result.category = "Audio Processing";
    result.value = throughputMB;
    result.unit = "MB/s";
    result.duration = duration;
    result.score = std::min(100.0f, (throughputMB / 50.0f) * 100.0f); // 50MB/s = 100
    
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    fBenchmark->AddResult(result);
}

void BenchmarkWindow::RunCategoryBenchmark(const char* category)
{
    if (fRunning) return;
    
    fRunning = true;
    fRunAllButton->SetEnabled(false);
    fAudioButton->SetEnabled(false);
    f3DButton->SetEnabled(false);
    fMemoryButton->SetEnabled(false);
    fSystemButton->SetEnabled(false);
    fStopButton->SetEnabled(true);
    fExportButton->SetEnabled(false);
    
    // Clear previous results
    ClearResults();
    
    // Create benchmark if needed
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    
    // Store category for thread
    fCurrentCategory = category;
    
    // Start benchmark in separate thread
    fStatusText->SetText("Running category benchmark tests...");
    fProgressBar->Reset();
    fProgressBar->SetMaxValue(100.0f);
    
    // Start category-specific benchmark thread
    fBenchmarkThread = spawn_thread(CategoryBenchmarkThreadEntry, "category_benchmark", 
                                   B_NORMAL_PRIORITY, this);
    if (fBenchmarkThread >= 0) {
        resume_thread(fBenchmarkThread);
    } else {
        // Fallback
        PostMessage(new BMessage(MSG_TEST_COMPLETE), this);
    }
}

// Static thread entry for category benchmarks
int32 BenchmarkWindow::CategoryBenchmarkThreadEntry(void* data)
{
    BenchmarkWindow* window = static_cast<BenchmarkWindow*>(data);
    window->RunCategoryTests();
    return B_OK;
}

void BenchmarkWindow::RunCategoryTests()
{
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    
    // Clear previous results
    fBenchmark->ClearResults();
    
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    
    if (fCurrentCategory == "audio") {
        // Run all audio tests
        progressMsg->AddFloat("progress", 10.0f);
        progressMsg->AddString("status", "Running audio tests...");
        PostMessage(progressMsg, this);
        
        TestAudioEngineSimple();
        
        progressMsg = new BMessage(MSG_TEST_UPDATE);
        progressMsg->AddFloat("progress", 30.0f);
        PostMessage(progressMsg, this);
        
        TestAudioLatency();
        
        progressMsg = new BMessage(MSG_TEST_UPDATE);
        progressMsg->AddFloat("progress", 60.0f);
        PostMessage(progressMsg, this);
        
        TestSineGeneration();
        
        progressMsg = new BMessage(MSG_TEST_UPDATE);
        progressMsg->AddFloat("progress", 90.0f);
        PostMessage(progressMsg, this);
        
        TestBufferProcessing();
        
    } else if (fCurrentCategory == "memory") {
        // Run memory tests
        progressMsg->AddFloat("progress", 10.0f);
        progressMsg->AddString("status", "Running memory tests...");
        PostMessage(progressMsg, this);
        
        TestMemoryBandwidth();
        
        progressMsg = new BMessage(MSG_TEST_UPDATE);
        progressMsg->AddFloat("progress", 50.0f);
        progressMsg->AddString("status", "Testing memory patterns...");
        PostMessage(progressMsg, this);
        
        TestMemoryPatterns();
        
    } else if (fCurrentCategory == "system") {
        // Run system tests  
        progressMsg->AddFloat("progress", 10.0f);
        progressMsg->AddString("status", "Running CPU tests...");
        PostMessage(progressMsg, this);
        
        TestCPUScaling();
        
        progressMsg = new BMessage(MSG_TEST_UPDATE);
        progressMsg->AddFloat("progress", 50.0f);
        progressMsg->AddString("status", "Testing realtime performance...");
        PostMessage(progressMsg, this);
        
        TestRealtimePerformance();
        
    } else if (fCurrentCategory == "3d") {
        // Simplified 3D test
        progressMsg->AddFloat("progress", 10.0f);
        progressMsg->AddString("status", "Running 3D simulation tests...");
        PostMessage(progressMsg, this);
        
        Test3DSimulation();
    }
    
    // Calculate final score
    if (fBenchmark) {
        float totalScore = 0.0f;
        const auto& results = fBenchmark->GetResults();
        if (!results.empty()) {
            for (const auto& result : results) {
                totalScore += result.score;
            }
            totalScore /= results.size();
            fBenchmark->SetTotalScore(totalScore);
        }
    }
    
    // Signal completion
    progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddFloat("progress", 100.0f);
    progressMsg->AddString("status", "Category tests complete!");
    PostMessage(progressMsg, this);
    
    PostMessage(new BMessage(MSG_TEST_COMPLETE), this);
}

void BenchmarkWindow::TestMemoryPatterns()
{
    // Test memory allocation patterns
    const int numAllocations = 1000;
    const int sizes[] = {1024, 4096, 16384, 65536, 262144}; // 1KB to 256KB
    const int numSizes = 5;
    
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddString("status", "Testing memory allocation patterns...");
    PostMessage(progressMsg, this);
    
    bigtime_t startTime = system_time();
    
    // Test allocation/deallocation speed
    for (int size : sizes) {
        std::vector<void*> allocations;
        
        // Allocate
        for (int i = 0; i < numAllocations; i++) {
            void* ptr = malloc(size);
            if (ptr) {
                memset(ptr, 0xAA, size); // Write pattern
                allocations.push_back(ptr);
            }
        }
        
        // Deallocate in reverse order (LIFO)
        while (!allocations.empty()) {
            free(allocations.back());
            allocations.pop_back();
        }
    }
    
    bigtime_t endTime = system_time();
    float duration = (endTime - startTime) / 1000.0f; // ms
    
    // Create result
    BenchmarkResult result;
    result.name = "Memory Allocation Patterns";
    result.category = "Memory";
    result.value = duration / (numAllocations * numSizes);
    result.unit = "ms/operation";
    result.duration = duration;
    result.score = std::max(0.0f, std::min(100.0f, 100.0f - duration / 10.0f));
    
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    fBenchmark->AddResult(result);
}

void BenchmarkWindow::TestRealtimePerformance()
{
    // Test realtime scheduling and missed deadlines
    const int numIterations = 100;
    const bigtime_t targetInterval = 10000; // 10ms intervals
    int missedDeadlines = 0;
    
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddString("status", "Testing realtime performance...");
    PostMessage(progressMsg, this);
    
    bigtime_t nextDeadline = system_time() + targetInterval;
    
    for (int i = 0; i < numIterations; i++) {
        // Simulate work
        volatile float work = 0;
        for (int j = 0; j < 10000; j++) {
            work += sinf(j * 0.001f);
        }
        
        bigtime_t currentTime = system_time();
        if (currentTime > nextDeadline) {
            missedDeadlines++;
        }
        
        // Wait for next deadline
        if (currentTime < nextDeadline) {
            snooze(nextDeadline - currentTime);
        }
        
        nextDeadline += targetInterval;
    }
    
    float successRate = ((numIterations - missedDeadlines) / (float)numIterations) * 100.0f;
    
    // Create result
    BenchmarkResult result;
    char name[256];
    snprintf(name, sizeof(name), "Realtime Performance (%d/%d deadlines met)", 
             numIterations - missedDeadlines, numIterations);
    result.name = name;
    result.category = "System";
    result.value = successRate;
    result.unit = "% success";
    result.duration = 0;
    result.score = successRate;
    
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    fBenchmark->AddResult(result);
}

void BenchmarkWindow::Test3DSimulation()
{
    // Mathematical 3D simulation (no OpenGL)
    const int numObjects = 100;
    const int numFrames = 60;
    
    BMessage* progressMsg = new BMessage(MSG_TEST_UPDATE);
    progressMsg->AddString("status", "Running 3D math simulation...");
    PostMessage(progressMsg, this);
    
    struct Vec3 { float x, y, z; };
    std::vector<Vec3> positions(numObjects);
    std::vector<Vec3> velocities(numObjects);
    
    // Initialize
    for (int i = 0; i < numObjects; i++) {
        positions[i] = {(float)(rand() % 100), (float)(rand() % 100), (float)(rand() % 100)};
        velocities[i] = {(float)(rand() % 10) * 0.1f, (float)(rand() % 10) * 0.1f, 0};
    }
    
    bigtime_t startTime = system_time();
    
    // Simulate physics
    for (int frame = 0; frame < numFrames; frame++) {
        for (int i = 0; i < numObjects; i++) {
            // Update position
            positions[i].x += velocities[i].x;
            positions[i].y += velocities[i].y;
            positions[i].z += velocities[i].z;
            
            // Simple collision with boundaries
            if (positions[i].x < 0 || positions[i].x > 100) velocities[i].x *= -1;
            if (positions[i].y < 0 || positions[i].y > 100) velocities[i].y *= -1;
            if (positions[i].z < 0 || positions[i].z > 100) velocities[i].z *= -1;
            
            // Matrix transform simulation
            float matrix[16];
            for (int j = 0; j < 16; j++) {
                matrix[j] = sinf(frame * 0.1f + i * 0.01f + j);
            }
        }
    }
    
    bigtime_t endTime = system_time();
    float duration = (endTime - startTime) / 1000.0f; // ms
    float fps = (numFrames * 1000.0f) / duration;
    
    // Create result
    BenchmarkResult result;
    char name[256];
    snprintf(name, sizeof(name), "3D Simulation (%d objects @ %.1f FPS)", numObjects, fps);
    result.name = name;
    result.category = "3D Graphics";
    result.value = fps;
    result.unit = "FPS";
    result.duration = duration;
    result.score = std::min(100.0f, (fps / 60.0f) * 100.0f);
    
    if (!fBenchmark) {
        fBenchmark = new PerformanceStation();
    }
    fBenchmark->AddResult(result);
}

void BenchmarkWindow::ExportTXT()
{
    BPath path;
    find_directory(B_DESKTOP_DIRECTORY, &path);
    path.Append("HaikuMix_Benchmark_Results.txt");
    
    BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (file.InitCheck() != B_OK) return;
    
    const auto& results = fBenchmark->GetResults();
    
    BString content;
    content << "HaikuMix Benchmark Results\n";
    content << "Generated: " << system_time() / 1000000 << "\n\n";
    
    // System info
    system_info sysInfo;
    get_system_info(&sysInfo);
    content << "System Information:\n";
    content << "CPU Cores: " << sysInfo.cpu_count << "\n";
    // CPU speed not available in system_info on all Haiku versions
    content << "Total RAM: " << sysInfo.max_pages * B_PAGE_SIZE / (1024 * 1024) << "MB\n";
    content << "Used RAM: " << sysInfo.used_pages * B_PAGE_SIZE / (1024 * 1024) << "MB\n";
    content << "Kernel: " << sysInfo.kernel_build_date << "\n\n";
    
    // Results by category
    std::map<std::string, std::vector<BenchmarkResult>> grouped;
    for (const auto& result : results) {
        grouped[result.category].push_back(result);
    }
    
    for (const auto& group : grouped) {
        content << "[" << group.first.c_str() << "]\n";
        for (const auto& result : group.second) {
            content << "  " << result.name.c_str() << ": " 
                   << result.value << " " << result.unit.c_str() 
                   << " (Score: " << result.score << "/100)\n";
        }
        content << "\n";
    }
    
    // Overall score
    float totalScore = fBenchmark->GetTotalScore();
    content << "Overall Score: " << totalScore << "/100\n";
    
    file.Write(content.String(), content.Length());
    
    BAlert* alert = new BAlert("Export Complete", 
        "TXT report exported to Desktop!", "OK");
    alert->Go();
}

void BenchmarkWindow::ExportHTML()
{
    BPath path;
    find_directory(B_DESKTOP_DIRECTORY, &path);
    path.Append("HaikuMix_Benchmark_Results.html");
    
    BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (file.InitCheck() != B_OK) return;
    
    const auto& results = fBenchmark->GetResults();
    
    BString html;
    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<title>HaikuMix Benchmark Results</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }\n";
    html << ".header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; border-radius: 10px; margin-bottom: 20px; }\n";
    html << ".section { background: white; padding: 15px; margin-bottom: 15px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }\n";
    html << ".score { font-size: 24px; font-weight: bold; color: #4CAF50; }\n";
    html << ".test-result { display: flex; justify-content: space-between; padding: 8px; margin: 5px 0; background: #f9f9f9; border-radius: 5px; }\n";
    html << ".category-header { background: #2196F3; color: white; padding: 10px; border-radius: 5px; font-weight: bold; }\n";
    html << "</style>\n";
    html << "</head>\n<body>\n";
    
    // Header
    html << "<div class='header'>\n";
    html << "<h1>🎵 HaikuMix Performance Benchmark</h1>\n";
    html << "<p>Complete system performance analysis for audio production</p>\n";
    html << "</div>\n";
    
    // System info
    system_info sysInfo;
    get_system_info(&sysInfo);
    html << "<div class='section'>\n";
    html << "<h2>📋 System Information</h2>\n";
    html << "<p><strong>CPU:</strong> " << sysInfo.cpu_count << " cores</p>\n";
    html << "<p><strong>RAM:</strong> " << sysInfo.max_pages * B_PAGE_SIZE / (1024 * 1024) << "MB total, " << sysInfo.used_pages * B_PAGE_SIZE / (1024 * 1024) << "MB used</p>\n";
    html << "<p><strong>Kernel:</strong> " << sysInfo.kernel_build_date << "</p>\n";
    html << "</div>\n";
    
    // Overall score
    float totalScore = fBenchmark->GetTotalScore();
    html << "<div class='section'>\n";
    html << "<h2>🎯 Overall Performance</h2>\n";
    html << "<div class='score'>" << totalScore << "/100</div>\n";
    html << "</div>\n";
    
    // Results by category
    std::map<std::string, std::vector<BenchmarkResult>> grouped;
    for (const auto& result : results) {
        grouped[result.category].push_back(result);
    }
    
    html << "<div class='section'>\n";
    html << "<h2>📊 Detailed Results</h2>\n";
    
    for (const auto& group : grouped) {
        html << "<div class='category-header'>" << group.first.c_str() << "</div>\n";
        for (const auto& result : group.second) {
            html << "<div class='test-result'>\n";
            html << "<span><strong>" << result.name.c_str() << "</strong></span>\n";
            html << "<span>" << result.value << " " << result.unit.c_str() 
                 << " (Score: " << result.score << "/100)</span>\n";
            html << "</div>\n";
        }
    }
    html << "</div>\n";
    
    html << "</body>\n</html>\n";
    
    file.Write(html.String(), html.Length());
    
    BAlert* alert = new BAlert("Export Complete", 
        "HTML report exported to Desktop!", "OK");
    alert->Go();
}

void BenchmarkWindow::ExportCSV()
{
    BPath path;
    find_directory(B_DESKTOP_DIRECTORY, &path);
    path.Append("HaikuMix_Benchmark_Results.csv");
    
    BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (file.InitCheck() != B_OK) return;
    
    const auto& results = fBenchmark->GetResults();
    
    BString csv;
    csv << "Test Name,Category,Value,Unit,Duration (ms),Score\n";
    
    for (const auto& result : results) {
        csv << "\"" << result.name.c_str() << "\",";
        csv << "\"" << result.category.c_str() << "\",";
        csv << result.value << ",";
        csv << "\"" << result.unit.c_str() << "\",";
        csv << result.duration << ",";
        csv << result.score << "\n";
    }
    
    file.Write(csv.String(), csv.Length());
    
    BAlert* alert = new BAlert("Export Complete", 
        "CSV data exported to Desktop!", "OK");
    alert->Go();
}

void BenchmarkWindow::SaveBenchmarkHistory()
{
    if (!fBenchmark) return;
    
    BPath historyPath;
    find_directory(B_USER_SETTINGS_DIRECTORY, &historyPath);
    historyPath.Append("HaikuMix");
    // Create directory if it doesn't exist
    BDirectory dir;
    if (dir.CreateDirectory(historyPath.Path(), &dir) != B_OK && 
        dir.SetTo(historyPath.Path()) != B_OK) {
        return; // Failed to create/access directory
    }
    
    // Create timestamped filename
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    char filename[64];
    strftime(filename, sizeof(filename), "benchmark_%Y%m%d_%H%M%S.json", tm);
    historyPath.Append(filename);
    
    BFile file(historyPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (file.InitCheck() != B_OK) return;
    
    // Create simple JSON
    BString json;
    json << "{\n";
    json << "  \"timestamp\": " << now << ",\n";
    json << "  \"date\": \"" << asctime(tm) << "\",\n";
    json << "  \"overall_score\": " << fBenchmark->GetTotalScore() << ",\n";
    json << "  \"results\": [\n";
    
    const auto& results = fBenchmark->GetResults();
    for (size_t i = 0; i < results.size(); i++) {
        const auto& result = results[i];
        json << "    {\n";
        json << "      \"name\": \"" << result.name.c_str() << "\",\n";
        json << "      \"category\": \"" << result.category.c_str() << "\",\n";
        json << "      \"value\": " << result.value << ",\n";
        json << "      \"unit\": \"" << result.unit.c_str() << "\",\n";
        json << "      \"score\": " << result.score << "\n";
        json << "    }";
        if (i < results.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  ]\n";
    json << "}\n";
    
    file.Write(json.String(), json.Length());
}

void BenchmarkWindow::ShowBenchmarkHistory()
{
    BPath historyPath;
    find_directory(B_USER_SETTINGS_DIRECTORY, &historyPath);
    historyPath.Append("HaikuMix");
    
    BDirectory historyDir(historyPath.Path());
    if (historyDir.InitCheck() != B_OK) {
        BAlert* alert = new BAlert("No History", 
            "No benchmark history found. Run some tests first!", "OK");
        alert->Go();
        return;
    }
    
    // Simple history display
    BString historyList;
    historyList << "Recent Benchmark History:\n\n";
    
    BEntry entry;
    int count = 0;
    while (historyDir.GetNextEntry(&entry) == B_OK && count < 5) {
        char name[B_FILE_NAME_LENGTH];
        entry.GetName(name);
        
        if (strstr(name, "benchmark_") == name) {
            // Extract date from filename
            char dateStr[32];
            sscanf(name, "benchmark_%8s_%6s.json", dateStr, dateStr + 9);
            dateStr[8] = '/';
            dateStr[11] = ':';
            
            historyList << "• " << dateStr << "\n";
            count++;
        }
    }
    
    if (count == 0) {
        historyList << "No previous benchmarks found.";
    } else {
        historyList << "\nHistory files saved in:\n" << historyPath.Path();
    }
    
    BAlert* alert = new BAlert("Benchmark History", historyList.String(), "OK");
    alert->Go();
}

} // namespace HaikuDAW