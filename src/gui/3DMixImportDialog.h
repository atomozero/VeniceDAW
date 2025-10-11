/*
 * 3DMixImportDialog.h - User interface for 3dmix project import configuration
 * Professional dialog for importing vintage BeOS 3dmix projects into VeniceDAW
 */

#ifndef THREEDMIX_IMPORT_DIALOG_H
#define THREEDMIX_IMPORT_DIALOG_H

#ifdef __HAIKU__
	#include <Window.h>
	#include <View.h>
	#include <Box.h>
	#include <Button.h>
	#include <CheckBox.h>
	#include <MenuField.h>
	#include <PopUpMenu.h>
	#include <MenuItem.h>
	#include <StringView.h>
	#include <TextControl.h>
	#include <ScrollView.h>
	#include <ListView.h>
	#include <FilePanel.h>
	#include <InterfaceDefs.h>
#else
	// Cross-platform headers for syntax checking
	#include "../testing/HaikuMockHeaders.h"
#endif
#include <ListItem.h>
#include <GroupLayout.h>
#include <GridLayout.h>
#include <LayoutBuilder.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include "../audio/3dmix/3DMixProjectImporter.h"
#include "../audio/3dmix/3DMixTestSuite.h"

// Forward declarations
class BRefFilter;

namespace VeniceDAW {

/*
 * Track preview item for import dialog
 */
class TrackPreviewItem : public BListItem {
public:
	TrackPreviewItem(const Track3DMix& track, const AudioFileResolution& resolution);
	virtual ~TrackPreviewItem();

	// BListItem overrides
	virtual void DrawItem(BView* owner, BRect frame, bool complete = false) override;
	virtual void Update(BView* owner, const BFont* font) override;

	// Data access
	const Track3DMix& GetTrack() const { return fTrack; }
	const AudioFileResolution& GetResolution() const { return fResolution; }
	bool IsSelected() const { return fSelected; }
	void SetSelected(bool selected) { fSelected = selected; }

private:
	void DrawTrackInfo(BView* owner, BRect frame);
	void DrawResolutionStatus(BView* owner, BRect frame);
	void DrawCoordinateInfo(BView* owner, BRect frame);

	Track3DMix fTrack;
	AudioFileResolution fResolution;
	bool fSelected;
	BBitmap* fStatusIcon;
};

/*
 * Coordinate system preview widget
 */
class CoordinatePreviewView : public BView {
public:
	CoordinatePreviewView();
	virtual ~CoordinatePreviewView();

	// BView overrides
	virtual void Draw(BRect updateRect) override;
	virtual void GetPreferredSize(float* width, float* height) override;
	virtual void MouseDown(BPoint where) override;

	// Configuration
	void SetConversionMode(coordinate_conversion_mode mode);
	void SetSpatialization(spatialization_standard standard);
	void SetTrackPositions(const std::vector<AudioSphericalCoordinate>& positions);

	// Interaction
	void SetSelectedTrack(int32 trackIndex);
	void ShowConversionPreview(bool show);

private:
	void DrawCoordinateSpace(BRect bounds);
	void DrawTrackPositions(BRect bounds);
	void DrawListenerPosition(BRect bounds);
	void DrawCoordinateGrid(BRect bounds);

	coordinate_conversion_mode fConversionMode;
	spatialization_standard fSpatialization;
	std::vector<AudioSphericalCoordinate> fTrackPositions;
	int32 fSelectedTrack;
	bool fShowPreview;
	BPoint fListenerPosition;
};

/*
 * Import configuration panel
 */
class ImportConfigPanel : public BView {
public:
	ImportConfigPanel();
	virtual ~ImportConfigPanel();

	// BView overrides
	virtual void AttachedToWindow() override;
	virtual void MessageReceived(BMessage* message) override;

	// Configuration access
	ImportConfiguration GetConfiguration() const;
	void SetConfiguration(const ImportConfiguration& config);

	// UI updates
	void UpdatePreview();
	void EnableAdvancedOptions(bool enable);

private:
	void CreateFileHandlingGroup();
	void CreateCoordinateGroup();
	void CreateAudioProcessingGroup();
	void CreateIntegrationGroup();

	// File handling controls
	BCheckBox* fResolvePathsCheck;
	BCheckBox* fConvertRawCheck;
	BCheckBox* fCopyToProjectCheck;
	BCheckBox* fCreateProjectDirCheck;

	// Coordinate conversion controls
	BMenuField* fConversionModeMenu;
	BMenuField* fSpatializationMenu;
	BCheckBox* fOptimizeBinauralCheck;
	BCheckBox* fPreserveOriginalCheck;

	// Audio processing controls
	BCheckBox* fNormalizeLevelsCheck;
	BCheckBox* fResampleCheck;
	BCheckBox* fEnableLoopingCheck;
	BCheckBox* fEnableEffectsCheck;

	// Integration controls
	BCheckBox* fOpen3DMixerCheck;
	BCheckBox* fAddToCurrentCheck;
	BCheckBox* fUpdateExistingCheck;

	// Message constants
	enum {
		MSG_CONFIG_CHANGED = 'cfgc',
		MSG_CONVERSION_MODE = 'cnvm',
		MSG_SPATIALIZATION = 'spat',
		MSG_PREVIEW_UPDATE = 'prev'
	};
};

/*
 * Main 3dmix import dialog
 */
class ThreeDMixImportDialog : public BWindow {
public:
	ThreeDMixImportDialog(const char* filePath, BWindow* parent = nullptr);
	virtual ~ThreeDMixImportDialog();

	// BWindow overrides
	virtual void MessageReceived(BMessage* message) override;
	virtual bool QuitRequested() override;
	virtual void WindowActivated(bool active) override;

	// Dialog results
	bool WasAccepted() const { return fAccepted; }
	ImportConfiguration GetConfiguration() const;
	std::vector<int32> GetSelectedTracks() const;

	// Project analysis
	bool AnalyzeProject();
	void UpdateProjectInfo();
	void UpdateTrackList();

private:
	void CreateInterface();
	void CreateProjectInfoPanel();
	void CreateTrackListPanel();
	void CreateConfigurationPanel();
	void CreatePreviewPanel();
	void CreateButtonPanel();

	// Event handlers
	void HandleImportClicked();
	void HandleCancelClicked();
	void HandlePreviewClicked();
	void HandleTrackSelectionChanged();
	void HandleConfigurationChanged();

	// UI updates
	void RefreshTrackList();
	void RefreshPreview();
	void UpdateImportButton();
	void ShowAnalysisProgress(bool show);

	// Project analysis
	void AnalyzeProjectAsync();
	void ProcessAnalysisResults();
	void ValidateImportRequirements();

	// Project data
	BString fFilePath;
	Project3DMix fProject;
	std::vector<AudioFileResolution> fResolutions;
	ThreeDMixProjectImporter fImporter;

	// Dialog state
	bool fAccepted;
	bool fAnalysisComplete;
	bool fValidProject;

	// UI components
	BView* fMainView;

	// Project info panel
	BBox* fProjectInfoBox;
	BStringView* fProjectNameView;
	BStringView* fTrackCountView;
	BStringView* fDurationView;
	BStringView* fSizeView;
	BStringView* fFormatView;

	// Track list panel
	BBox* fTrackListBox;
	BListView* fTrackList;
	BScrollView* fTrackScrollView;
	BButton* fSelectAllButton;
	BButton* fSelectNoneButton;

	// Configuration panel
	BBox* fConfigBox;
	ImportConfigPanel* fConfigPanel;

	// Preview panel
	BBox* fPreviewBox;
	CoordinatePreviewView* fPreviewView;
	BStringView* fPreviewStatus;

	// Button panel
	BView* fButtonPanel;
	BButton* fImportButton;
	BButton* fCancelButton;
	BButton* fPreviewButton;
	BButton* fAdvancedButton;

	// Progress indication
	BStringView* fProgressView;

	// Message constants
	enum {
		MSG_IMPORT_CLICKED = 'impc',
		MSG_CANCEL_CLICKED = 'canc',
		MSG_PREVIEW_CLICKED = 'prvc',
		MSG_ADVANCED_CLICKED = 'advc',
		MSG_SELECT_ALL_TRACKS = 'sall',
		MSG_SELECT_NO_TRACKS = 'snon',
		MSG_TRACK_SELECTED = 'trks',
		MSG_ANALYSIS_COMPLETE = 'anlc',
		MSG_CONFIG_UPDATED = 'cfgu'
	};
};

/*
 * Import progress window
 */
class ThreeDMixImportProgress : public BWindow {
public:
	ThreeDMixImportProgress(BWindow* parent);
	virtual ~ThreeDMixImportProgress();

	// BWindow overrides
	virtual void MessageReceived(BMessage* message) override;
	virtual bool QuitRequested() override;

	// Progress updates
	void SetOperation(const char* operation);
	void SetProgress(float progress);
	void SetTrackInfo(const char* trackName, int32 current, int32 total);
	void SetStatus(const char* status);

	// Error handling
	void ShowError(const char* error);
	void ShowWarning(const char* warning);

	// Control
	void EnableCancelButton(bool enable);
	bool WasCancelled() const { return fCancelled; }

	// Completion
	void ShowCompleted(bool success, const char* message);

private:
	void CreateInterface();
	void UpdateProgressBar();

	// State
	bool fCancelled;
	float fCurrentProgress;

	// UI components
	BView* fMainView;
	BStringView* fOperationView;
	BStringView* fTrackInfoView;
	BStringView* fStatusView;
	BView* fProgressBar; // Custom progress bar
	BButton* fCancelButton;

	// Visual elements
	BBitmap* fIconBitmap;

	// Message constants
	enum {
		MSG_CANCEL_IMPORT = 'cani',
		MSG_UPDATE_PROGRESS = 'updp'
	};
};

/*
 * 3dmix file filter for file panels
 */
class ThreeDMixFileFilter : public BRefFilter {
public:
	ThreeDMixFileFilter();
	virtual ~ThreeDMixFileFilter();

	virtual bool Filter(const entry_ref* ref, BNode* node,
	                   struct stat_beos* stat, const char* mimeType) override;
};

/*
 * Utility functions for 3dmix UI integration
 */
class ThreeDMixUIUtils {
public:
	// Dialog creation
	static ThreeDMixImportDialog* ShowImportDialog(const char* filePath, BWindow* parent = nullptr);
	static ThreeDMixImportProgress* ShowProgressDialog(BWindow* parent = nullptr);

	// File panels
	static BFilePanel* CreateImportFilePanel(BWindow* target);
	static BFilePanel* CreateExportFilePanel(BWindow* target);

	// Icons and graphics
	static BBitmap* CreateThreeDMixIcon(icon_size size = B_LARGE_ICON);
	static BBitmap* CreateTrackStatusIcon(bool resolved, icon_size size = B_MINI_ICON);
	static BBitmap* CreateCoordinatePreviewIcon();

	// Menu integration
	static BMenu* CreateImportMenu(BWindow* target);
	static void AddThreeDMixMenuItems(BMenu* menu, BWindow* target);

	// Color schemes
	static rgb_color GetResolvedColor();
	static rgb_color GetUnresolvedColor();
	static rgb_color GetWarningColor();
	static rgb_color GetSuccessColor();

	// String formatting
	static BString FormatTrackInfo(const Track3DMix& track);
	static BString FormatCoordinate(const Coordinate3D& coord);
	static BString FormatSphericalCoordinate(const AudioSphericalCoordinate& coord);
	static BString FormatFileSize(off_t size);
	static BString FormatDuration(float seconds);
};

} // namespace VeniceDAW

#endif // THREEDMIX_IMPORT_DIALOG_H