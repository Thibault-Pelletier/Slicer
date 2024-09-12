#include "vtkSlicerLibrary.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkSlicerLibrary);

vtkSlicerLibrary::vtkSlicerLibrary() = default;
vtkSlicerLibrary::~vtkSlicerLibrary() = default;

int vtkSlicerLibrary::GetVersionMajor()
{
    return 1;
}

int vtkSlicerLibrary::GetVersionMinor()
{
    return 0;
}

int vtkSlicerLibrary::GetVersionPatch()
{
    return 0;
}

const char* vtkSlicerLibrary::GetVersionString()
{
    return "1.0.0";
}
