#include <Core.h>

int main() {
	Core::BeginPlay("FieldServer", ProcessType::Headless);

	PRINT("FieldServer Initialized (Headless Mode)!");

	DOUBLE lastLogTime = 0.0;
	INT ticks = 0;

	// Server loop (runs with zero GPU / Window overhead)
	while (Core::Tick()) {
		ticks++;

		if (M_Time::Check_Cooldown(lastLogTime, 1.0)) {
			PRINT("FieldServer alive | Tick Rate: {} TPS | Delta: {:.4f}s", ticks, M_Time::Get_DeltaTime());
			ticks = 0;
		}

		// Dedicated server sleep to prevent 100% core burn while idle
		M_Time::Sleep(1);
	}

	Core::EndPlay();
    return 0;
}
