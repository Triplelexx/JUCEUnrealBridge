/*
	JUCE Unreal Bridge 0.0.1
	----
	Sean Soraghan - ROLI
*/

#pragma once

#include "JUCEUnrealBridgePCH.h"
#include "TimeTickerComponent.h"
#include "MetronomeComponent.generated.h"


//================================================================================
UENUM (BlueprintType)
enum class RhythmicUnitType : uint8
{
    Sixteenth UMETA(DisplayName = "Sixteenth"),
    Eighth 	  UMETA(DisplayName = "Eighth"),
	Beat	  UMETA(DisplayName = "Beat")
};
//================================================================================
//================================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMetronomeSixteenth, int, index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMetronomeEighth,    int, index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMetronomeBeat,      int, index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMetronomeBar,       int, index);

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup="JUCE-Components")
class JUCEUNREALBRIDGE_API UMetronomeComponent : public UTimeTickerComponent
{
	GENERATED_BODY()

public:
    class Listener
    {
    public:
        virtual void SixteenthTicked (int index) {}
        virtual void EigthTicked     (int index) {}
        virtual void BeatTicked      (int index) {}
        virtual void BarTicked       (int index) {}
    };

private:
    juce::ListenerList<Listener> Listeners;
    void AddListener    (Listener* listener) { Listeners.add    (listener); }
    void RemoveListener (Listener* listener) { Listeners.remove (listener); }

    struct RhythmicUnit
    {
        RhythmicUnit() 
        : CurrentIndex (-1), NumSubdivisions (4)
        {}

        RhythmicUnit (int nSubdivisions) 
        : CurrentIndex (-1), NumSubdivisions (nSubdivisions)
        {}

        void Tick()
        {
            CurrentIndex = (CurrentIndex + 1) % NumSubdivisions;
            if (TickCallback != 0)
                TickCallback (CurrentIndex);
        }

        std::function<void (int index)> TickCallback;
        int CurrentIndex;
        int NumSubdivisions;
    };

public:
	void BeginPlay() override;
    FORCEINLINE void OnComponentDestroyed (bool bDestroyingHierarchy) override
    {
        Super::OnComponentDestroyed (bDestroyingHierarchy);
        StopMetronome();
    }

	UPROPERTY(BlueprintAssignable, Category="JUCE-Metronome")
    FMetronomeSixteenth OnSixteenth;
    UPROPERTY(BlueprintAssignable, Category="JUCE-Metronome")
    FMetronomeEighth OnEighth;
    UPROPERTY(BlueprintAssignable, Category="JUCE-Metronome")
    FMetronomeBeat OnBeat;
    UPROPERTY(BlueprintAssignable, Category="JUCE-Metronome")
    FMetronomeBar OnBar;

    virtual void SixteenthTickCallback (int index);
    virtual void EighthTickCallback    (int index);
    virtual void BeatTickCallback      (int index);
    virtual void BarTickCallback       (int index);

    UFUNCTION (BlueprintCallable, Category = "JUCE-Metronome")
    void StartMetronome()
    {
        MetronomeRunning = true;
    }
    UFUNCTION (BlueprintCallable, Category = "JUCE-Metronome")
    void StopMetronome()
    {
        MetronomeRunning = false;
    }

    UFUNCTION (BlueprintCallable, Category = "JUCE-Metronome")
    void SetTempo (float bpm)
    {
    	double beatsPerSecond      = bpm / 60.0;
    	double secondsPerSixteenth = 1.0 / (beatsPerSecond * 4.0f);
    	SetTickTime ((float) secondsPerSixteenth);
    } 

    UFUNCTION (BlueprintCallable, Category = "JUCE-Metronome")
    void SetTimeSignature (int numerator, RhythmicUnitType divisor) 
    {
    	Numerator = numerator;
    	Divisor   = divisor;
    	UpdateSubdivisions();
    }

private:
    bool MetronomeRunning = false;
	int  Numerator        = 4;
	RhythmicUnitType Divisor = RhythmicUnitType::Beat;

    RhythmicUnit Sixteenth;
    RhythmicUnit Eighth;
    RhythmicUnit Beat;
    RhythmicUnit Bar;

    FORCEINLINE void UpdateSubdivisions()
    {
    	Sixteenth.NumSubdivisions = 2;
    	Eighth.NumSubdivisions    = 2;
    	Beat.NumSubdivisions      = 4;
    	switch (Divisor)
    	{
    		case RhythmicUnitType::Sixteenth: Sixteenth.NumSubdivisions = Numerator; break;
    		case RhythmicUnitType::Eighth:    Eighth.NumSubdivisions    = Numerator; break;
    		case RhythmicUnitType::Beat:      Beat.NumSubdivisions      = Numerator; break;
    	}
    }

    FORCEINLINE void TickMetronome()
    {
        if (MetronomeRunning)
        {
    	    switch (Divisor)
    	    {
    		    case RhythmicUnitType::Sixteenth: TickRhythmicUnit (Sixteenth, Bar);               break;
    		    case RhythmicUnitType::Eighth:    TickRhythmicUnit (Sixteenth, Eighth, Bar);       break;
    		    case RhythmicUnitType::Beat:      TickRhythmicUnit (Sixteenth, Eighth, Beat, Bar); break;
    	    }
        }
    }

    template<typename R>
    FORCEINLINE void TickRhythmicUnit (R& unit) { unit.Tick(); }

    template<typename R, typename... Args>
    FORCEINLINE void TickRhythmicUnit (R& unit, Args&... args) 
    {
        unit.Tick();
        if (unit.CurrentIndex == 0)
            TickRhythmicUnit (args...);
    }
};