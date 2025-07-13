#include "vtkMouseCursor.h"
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkMouseCursor);

vtkMouseCursor::vtkMouseCursor() = default;
vtkMouseCursor::~vtkMouseCursor() = default;

void vtkMouseCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CursorShape: " << GetCursorPath() << "\n";
  os << indent << "CursorPath: " << GetCursorPath() << "\n";
  os << indent << "HotX: " << GetHotX() << "\n";
  os << indent << "HotY: " << GetHotY() << "\n";
}

void vtkMouseCursor::SetShape(CursorShape shape)
{
  CursorData = shape;
  Modified();
}

void vtkMouseCursor::SetPath(const std::string& path, int hotX, int hotY)
{
  CursorData = path;
  HotX = hotX;
  HotY = hotY;
  Modified();
}

int vtkMouseCursor::GetCursorShape() const
{
  if (std::holds_alternative<int>(CursorData))
    return std::get<int>(CursorData);
  return NoCursor;
}

std::string vtkMouseCursor::GetCursorPath() const
{
  if (std::holds_alternative<std::string>(CursorData))
  {
    return std::get<std::string>(CursorData);
  }
  return {};
}
