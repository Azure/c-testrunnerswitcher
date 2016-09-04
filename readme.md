This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# testrunnerswitcher

testrunnerswitcher is a simple library to switch test runners between azure-ctest (available at https://github.com/Azure/azure-ctest.git) and CppUnitTest.

## Setup

- Clone azure-uamqp-c by:
```
git clone --recursive https://github.com/Azure/azure-c-testrunnerswitcher.git
```
- Create a cmake folder under azure-c-testrunnerswitcher
- Switch to the cmake folder and run
   cmake ..
- Build the code for your platform (msbuild for Windows, make for Linux, etc.)
