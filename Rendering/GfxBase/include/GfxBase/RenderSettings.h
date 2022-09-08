#pragma once

#include <vector>

#include "util/G3DCore.h"

namespace RBX {

class CRenderSettings
{
public:
	enum AASamples
	{
		NONE = 1,
		AA4 = 4,
		AA8 = 8,
	};

	static const AASamples defaultAASamples = NONE;
	static const G3D::Vector2int16 defaultWindowSize;
	static const G3D::Vector2int16 defaultFullscreenSize();
	static const G3D::Vector2int16 minGameWindowSize;

	typedef enum
	{
		UnknownGraphicsMode = 0,
		AutoGraphicsMode = 1,
        Direct3D11 = 2,
		Direct3D9 = 3,
		OpenGL,
		NoGraphics
	} GraphicsMode;

	static GraphicsMode latchedGraphicsMode;

	typedef enum
	{
		AntialiasingAuto = 0,
		AntialiasingOn = 1,
		AntialiasingOff = 2
	} AntialiasingMode;

	typedef enum
	{
		FrameRateManagerAuto = 0,
		FrameRateManagerOn = 1,
		FrameRateManagerOff = 2
	} FrameRateManagerMode;

	typedef enum
	{
		QualityAuto = 0,
		QualityLevel1,
		QualityLevel2,
		QualityLevel3,
		QualityLevel4,
		QualityLevel5,
		QualityLevel6,
		QualityLevel7,
		QualityLevel8,
		QualityLevel9,
		QualityLevel10,
		QualityLevel11,
		QualityLevel12,
		QualityLevel13,
		QualityLevel14,
		QualityLevel15,
		QualityLevel16,
		QualityLevel17,
		QualityLevel18,
		QualityLevel19,
		QualityLevel20,
		QualityLevel21,
		QualityLevelMax
	} QualityLevel;

	typedef enum
	{
		ResolutionAuto,
		Resolution720x526,
		Resolution800x600,

		Resolution1024x600,
		Resolution1024x768,

		Resolution1280x720,
		Resolution1280x768,
		Resolution1152x864,
		Resolution1280x800,
		Resolution1360x768,
		Resolution1280x960,
		Resolution1280x1024,

		Resolution1440x900,
		Resolution1600x900,
		Resolution1600x1024,
		Resolution1600x1200,
		Resolution1680x1050,

		Resolution1920x1080,
		Resolution1920x1200,

		ResolutionMaxIndex
	} ResolutionPreset;

	struct RESOLUTIONENTRY
	{
		CRenderSettings::ResolutionPreset preset;
		int width;
		int height;
	};

protected:
	GraphicsMode graphicsMode;
	AntialiasingMode antialiasingMode;
	FrameRateManagerMode frameRateManagerMode;
	QualityLevel qualityLevel;
	QualityLevel editQualityLevel;

	ResolutionPreset resolutionPreference;

	int				autoQualityLevel;
	int				maxQualityLevel;
	int				minCullDistance;
	bool			debugShowBoundingBoxes;
    bool            debugReloadAssets;
	bool			enableFRM;
    bool            objExportMergeByMaterial;

	static AASamples aaSamples;

	// filtered setting to use by app.
	G3D::Vector2int16 fullscreenSize;
	G3D::Vector2int16 windowSize;
    
	bool showAggregation;

	bool drawConnectors;

	bool eagerBulkExecution;

	// KB
	unsigned int textureCacheSize;
	unsigned int meshCacheSize;
    
public:
	CRenderSettings();

	bool getShowAggregation() const { return showAggregation; }

	static AASamples getAASamplesSafe() { return aaSamples; }	// Thread-safe

	GraphicsMode getGraphicsMode() const { return graphicsMode; }
	void setGraphicsMode(GraphicsMode value);

	GraphicsMode getLatchedGraphicsMode()
	{
		if (latchedGraphicsMode == UnknownGraphicsMode)
			latchedGraphicsMode = getGraphicsMode();
		return latchedGraphicsMode;
	}

	AASamples getAASamples() const { return aaSamples; }

	G3D::Vector2int16 getFullscreenSize() const { return fullscreenSize; }
	G3D::Vector2int16 getWindowSize() const { return windowSize; }

	FrameRateManagerMode getFrameRateManagerMode() const { return frameRateManagerMode; }
	AntialiasingMode getAntialiasingMode() const { return antialiasingMode; }

	QualityLevel getQualityLevel() const { return qualityLevel; }
	QualityLevel getEditQualityLevel() const { return editQualityLevel; }
	int getMaxQualityLevel() { return maxQualityLevel; }
	int getAutoQualityLevel() const { return autoQualityLevel; }

	ResolutionPreset getResolutionPreference() const { return resolutionPreference; }
	const RESOLUTIONENTRY& getResolutionPreset(ResolutionPreset preset) const;

	// FRM would like to report latest setting. Subclass is free to ignore it
	virtual void setAutoQualityLevel(int level) {}

	float getMaxFrameRate() const { return 300.0f; }
	float getMinFrameRate() const { return 30.0f; }

	bool getDrawConnectors() const { return drawConnectors; }
    void setDrawConnectors(bool value) { drawConnectors = value; }

	int getMinCullDistance() const { return minCullDistance; }
	bool getDebugShowBoundingBoxes() const { return debugShowBoundingBoxes; }
	bool getDebugReloadAssets() const { return debugReloadAssets; }
    bool getObjExportMergeByMaterial() const { return objExportMergeByMaterial; }
	bool getEnableFRM() const { return enableFRM; }
	
	bool getEagerBulkExecution() const { return eagerBulkExecution; }

	unsigned int getTextureCacheSize() const { return textureCacheSize; }
	unsigned int getMeshCacheSize() const { return meshCacheSize; }
};

}  // namespace RBX
