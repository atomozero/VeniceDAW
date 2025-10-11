/*
 * 3DMixTestSuite.h - Comprehensive testing infrastructure for 3dmix import
 * Validates all components: parser, coordinate mapper, path resolver, and integration
 */

#ifndef THREEDMIX_TEST_SUITE_H
#define THREEDMIX_TEST_SUITE_H

#include "3DMixFormat.h"
#include "3DMixParser.h"
#include "CoordinateSystemMapper.h"
#include "AudioPathResolver.h"
#include "3DMixProjectImporter.h"
#include <support/String.h>
#include <vector>

namespace VeniceDAW {

/*
 * Test result classification
 */
enum test_result_type {
	TEST_PASSED = 0,
	TEST_FAILED,
	TEST_SKIPPED,
	TEST_WARNING
};

/*
 * Individual test result
 */
struct TestResult {
	BString testName;
	test_result_type result;
	BString message;
	bigtime_t executionTime;
	BString details;

	TestResult(const char* name)
		: testName(name), result(TEST_SKIPPED), executionTime(0) {}

	TestResult(const char* name, test_result_type res, const char* msg)
		: testName(name), result(res), message(msg), executionTime(0) {}
};

/*
 * Test suite statistics
 */
struct TestSuiteStats {
	int32 totalTests;
	int32 passedTests;
	int32 failedTests;
	int32 skippedTests;
	int32 warningTests;
	bigtime_t totalTime;
	float successRate;

	TestSuiteStats()
		: totalTests(0), passedTests(0), failedTests(0)
		, skippedTests(0), warningTests(0), totalTime(0), successRate(0.0f) {}
};

/*
 * Mock 3dmix data for testing
 */
class Mock3DMixData {
public:
	static Project3DMix CreateTestProject();
	static Track3DMix CreateTestTrack(const char* name, float x, float y, float z);
	static std::vector<uint8> CreateMockBMessageData();
	static std::vector<uint8> CreateValidMagicHeader(int32 trackCount);

	// Test file creation
	static bool CreateMockAudioFile(const BString& path, const AudioFormatDetection& format);
	static bool CreateMockRawFile(const BString& path, int32 sampleRate, int32 channels, float duration);
	static bool CreateMock3DMixFile(const BString& path, const Project3DMix& project);

	// Sample data sets
	static std::vector<Coordinate3D> GetSampleBeOSCoordinates();
	static std::vector<AudioFormatDetection> GetSampleAudioFormats();
	static std::vector<BString> GetSampleBeOSPaths();
};

/*
 * Core format and data structure tests
 */
class FormatTests {
public:
	static std::vector<TestResult> RunAllTests();

	// Coordinate system tests
	static TestResult TestCoordinate3DValidation();
	static TestResult TestSphericalCoordinateConversion();
	static TestResult TestCoordinateNormalization();
	static TestResult TestCoordinateBoundaryConditions();

	// Audio format tests
	static TestResult TestAudioFormat3DMixValidation();
	static TestResult TestMediaFormatConversion();
	static TestResult TestAudioFormatDetection();

	// Track tests
	static TestResult TestTrack3DMixCreation();
	static TestResult TestTrackParameterValidation();
	static TestResult TestTrackSphericalPositioning();

	// Project tests
	static TestResult TestProject3DMixManagement();
	static TestResult TestProjectValidation();
	static TestResult TestProjectStatistics();
};

/*
 * Parser and file format tests
 */
class ParserTests {
public:
	static std::vector<TestResult> RunAllTests();

	// BMessage parsing tests
	static TestResult TestBMessageParsing();
	static TestResult TestBMessageTypeCodeHandling();
	static TestResult TestBMessageDataExtraction();
	static TestResult TestBMessageErrorHandling();

	// 3dmix file format tests
	static TestResult TestMagicNumberValidation();
	static TestResult TestHeaderParsing();
	static TestResult TestTrackRecordParsing();
	static TestResult TestPointerFileResolution();

	// Legacy project loading tests
	static TestResult TestProjectLoading();
	static TestResult TestProjectValidation();
	static TestResult TestErrorRecovery();
	static TestResult TestLargeProjectHandling();
};

/*
 * Coordinate conversion tests
 */
class CoordinateTests {
public:
	static std::vector<TestResult> RunAllTests();

	// Conversion algorithm tests
	static TestResult TestDirectScaleConversion();
	static TestResult TestSphericalConversion();
	static TestResult TestCylindricalConversion();
	static TestResult TestAmbisonicsConversion();

	// Coordinate space transformation tests
	static TestResult TestBeOSToModernConversion();
	static TestResult TestListenerTransformation();
	static TestResult TestWorkspaceMapping();
	static TestResult TestBoundaryConditions();

	// Audio-specific optimization tests
	static TestResult TestBinauralOptimization();
	static TestResult TestSpatializationHints();
	static TestResult TestDistanceCalculations();
	static TestResult TestAttenuationModels();

	// Batch processing tests
	static TestResult TestProjectCoordinateConversion();
	static TestResult TestPerformanceMetrics();
	static TestResult TestPrecisionValidation();
};

/*
 * Path resolution tests
 */
class PathResolverTests {
public:
	static std::vector<TestResult> RunAllTests();

	// Path translation tests
	static TestResult TestBeOSPathTranslation();
	static TestResult TestTranslationRules();
	static TestResult TestPathNormalization();
	static TestResult TestRelativePathHandling();

	// File search tests
	static TestResult TestExactPathResolution();
	static TestResult TestFilenameSearch();
	static TestResult TestFuzzyMatching();
	static TestResult TestRecursiveSearch();

	// Audio format detection tests
	static TestResult TestRawAudioDetection();
	static TestResult TestFormatHeuristics();
	static TestResult TestAudioFileValidation();
	static TestResult TestFormatConversion();

	// Performance tests
	static TestResult TestCachePerformance();
	static TestResult TestSearchTimeout();
	static TestResult TestBatchResolution();
};

/*
 * Integration tests
 */
class IntegrationTests {
public:
	static std::vector<TestResult> RunAllTests();

	// Project import tests
	static TestResult TestCompleteProjectImport();
	static TestResult TestImportConfiguration();
	static TestResult TestTrackMapping();
	static TestResult TestAudioFileProcessing();

	// VeniceDAW integration tests
	static TestResult TestMixerIntegration();
	static TestResult Test3DMixerIntegration();
	static TestResult TestAudioEngineIntegration();
	static TestResult TestUIIntegration();

	// Error handling tests
	static TestResult TestMissingFileHandling();
	static TestResult TestCorruptedDataHandling();
	static TestResult TestPartialImportRecovery();
	static TestResult TestValidationFailureHandling();

	// Performance tests
	static TestResult TestLargeProjectImport();
	static TestResult TestMemoryUsage();
	static TestResult TestImportSpeed();
};

/*
 * Regression tests using real 3dmix files
 */
class RegressionTests {
public:
	static std::vector<TestResult> RunAllTests();

	// Known project tests (based on reverse engineering)
	static TestResult TestSheLovesItProject();
	static TestResult TestTheLynxProject();
	static TestResult TestThePriceOfThingsProject();

	// Edge case tests
	static TestResult TestEmptyProject();
	static TestResult TestSingleTrackProject();
	static TestResult TestMaximumTrackProject();
	static TestResult TestCorruptedProject();

	// Compatibility tests
	static TestResult TestDifferentBeOSVersions();
	static TestResult TestDifferentAudioFormats();
	static TestResult TestDifferentCoordinateRanges();
};

/*
 * Performance benchmark tests
 */
class PerformanceTests {
public:
	static std::vector<TestResult> RunAllTests();

	// Component performance tests
	static TestResult BenchmarkParsing();
	static TestResult BenchmarkCoordinateConversion();
	static TestResult BenchmarkPathResolution();
	static TestResult BenchmarkAudioProcessing();

	// Memory usage tests
	static TestResult TestMemoryLeaks();
	static TestResult TestMemoryEfficiency();
	static TestResult TestLargeProjectMemory();

	// Scalability tests
	static TestResult TestMultipleProjectsHandling();
	static TestResult TestConcurrentImports();
	static TestResult TestResourceLimits();
};

/*
 * Main test suite orchestrator
 */
class ThreeDMixTestSuite {
public:
	ThreeDMixTestSuite();
	~ThreeDMixTestSuite();

	// Test execution
	bool RunAllTests();
	bool RunTestCategory(const char* category);
	bool RunSingleTest(const char* testName);

	// Test configuration
	void SetVerboseOutput(bool verbose) { fVerboseOutput = verbose; }
	void SetStopOnFirstFailure(bool stop) { fStopOnFirstFailure = stop; }
	void SetOutputFile(const char* filename) { fOutputFile.SetTo(filename); }

	// Test results
	const std::vector<TestResult>& GetAllResults() const { return fAllResults; }
	TestSuiteStats GetStatistics() const { return fStats; }
	bool HasFailures() const { return fStats.failedTests > 0; }

	// Test reporting
	void PrintSummaryReport();
	void PrintDetailedReport();
	void ExportTestReport(const char* filename);

	// Mock data management
	void SetupTestEnvironment();
	void CleanupTestEnvironment();

private:
	// Test execution helpers
	bool ExecuteTestCategory(const char* category, std::vector<TestResult> (*testFunction)());
	void ProcessTestResults(const std::vector<TestResult>& results);
	void UpdateStatistics();

	// Test environment setup
	void CreateTestDirectories();
	void CreateTestAudioFiles();
	void CreateTest3DMixFiles();
	void CleanupTestFiles();

	// Reporting helpers
	void PrintTestResult(const TestResult& result);
	BString FormatExecutionTime(bigtime_t time);
	BString GetResultString(test_result_type result);

	// Configuration
	bool fVerboseOutput;
	bool fStopOnFirstFailure;
	BString fOutputFile;

	// Test state
	std::vector<TestResult> fAllResults;
	TestSuiteStats fStats;
	std::vector<BString> fTestDirectories;
	std::vector<BString> fTestFiles;

	// Test categories
	struct TestCategory {
		BString name;
		std::vector<TestResult> (*testFunction)();
		bool enabled;
	};
	std::vector<TestCategory> fTestCategories;
};

/*
 * Test utilities and helpers
 */
class TestUtils {
public:
	// File system utilities
	static bool CreateDirectory(const BString& path);
	static bool FileExists(const BString& path);
	static bool RemoveFile(const BString& path);
	static bool CompareFiles(const BString& file1, const BString& file2);

	// Data validation utilities
	static bool ValidateCoordinate(const Coordinate3D& coord, float tolerance = 0.001f);
	static bool ValidateAudioFormat(const AudioFormat3DMix& format);
	static bool ValidateProject(const Project3DMix& project);

	// Mock data utilities
	static std::vector<uint8> GenerateRandomAudioData(size_t length);
	static Project3DMix GenerateRandomProject(int32 trackCount);
	static Track3DMix GenerateRandomTrack();

	// Performance measurement
	static bigtime_t MeasureExecutionTime(void (*function)());
	static void LogMemoryUsage(const char* context);

	// String utilities
	static BString FormatBytes(off_t bytes);
	static BString FormatTime(bigtime_t microseconds);
	static BString FormatFloat(float value, int32 precision = 3);
};

/*
 * Test assertion macros
 */
#define TEST_ASSERT(condition, message) \
	do { \
		if (!(condition)) { \
			return TestResult(__func__, TEST_FAILED, message); \
		} \
	} while (0)

#define TEST_ASSERT_EQUAL(expected, actual, message) \
	do { \
		if ((expected) != (actual)) { \
			BString error; \
			error.SetToFormat("%s (expected: %s, actual: %s)", \
			                  message, BString() << (expected), BString() << (actual)); \
			return TestResult(__func__, TEST_FAILED, error.String()); \
		} \
	} while (0)

#define TEST_ASSERT_NEAR(expected, actual, tolerance, message) \
	do { \
		if (fabs((expected) - (actual)) > (tolerance)) { \
			BString error; \
			error.SetToFormat("%s (expected: %.6f, actual: %.6f, tolerance: %.6f)", \
			                  message, (expected), (actual), (tolerance)); \
			return TestResult(__func__, TEST_FAILED, error.String()); \
		} \
	} while (0)

#define TEST_WARN(condition, message) \
	do { \
		if (!(condition)) { \
			return TestResult(__func__, TEST_WARNING, message); \
		} \
	} while (0)

} // namespace VeniceDAW

#endif // THREEDMIX_TEST_SUITE_H