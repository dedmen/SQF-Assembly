class CfgPatches
{
	class dedmen_sqf_assembly
	{
		name = "sqf-assembly";
		units[] = {};
		weapons[] = {};
		requiredVersion = 1.76;
		requiredAddons[] = {"intercept_core"};
		author = "Dedmen";
		authors[] = {"Dedmen"};
		url = "https://github.com/dedmen/SQF-Assembly";
		version = "1.0";
		versionStr = "1.0";
		versionAr[] = {1,0};
	};
};
class Intercept {
    class Dedmen {
        class SQF_Assembly {
			//certificate = "\z\SQF-Assembly\addons\main\dummycert";
            pluginName = "SQF-Assembly";
        };
    };
};