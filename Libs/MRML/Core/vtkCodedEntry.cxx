/*=auto=========================================================================

Portions (c) Copyright 2017 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

#include "vtkCodedEntry.h"

// VTK include
#include <vtkObjectFactory.h>

// STD include
#include <sstream>

vtkStandardNewMacro(vtkCodedEntry);

//----------------------------------------------------------------------------
vtkCodedEntry::vtkCodedEntry() = default;

//----------------------------------------------------------------------------
vtkCodedEntry::~vtkCodedEntry()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
void vtkCodedEntry::Initialize()
{
  this->SetCodeValue(nullptr);
  this->SetCodingSchemeDesignator(nullptr);
  this->SetCodeMeaning(nullptr);
}

//----------------------------------------------------------------------------
void vtkCodedEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "CodeValue: " << (this->CodeValue ? this->CodeValue : "(none)") << "\n";
  os << indent << "CodingSchemeDesignator: " << (this->CodingSchemeDesignator ? this->CodingSchemeDesignator : "(none)") << "\n";
  os << indent << "CodeMeaning: " << (this->CodeMeaning ? this->CodeMeaning : "(none)") << "\n";
}

//----------------------------------------------------------------------------
void vtkCodedEntry::Copy(vtkCodedEntry* aEntry)
{
  if (!aEntry)
  {
    return;
  }
  this->SetCodeValue(aEntry->GetCodeValue());
  this->SetCodingSchemeDesignator(aEntry->GetCodingSchemeDesignator());
  this->SetCodeMeaning(aEntry->GetCodeMeaning());
}

//----------------------------------------------------------------------------
void vtkCodedEntry::SetValueSchemeMeaning(const std::string& value, const std::string& scheme, const std::string& meaning)
{
  this->SetCodeValue(value.c_str());
  this->SetCodingSchemeDesignator(scheme.c_str());
  this->SetCodeMeaning(meaning.c_str());
}

//----------------------------------------------------------------------------
bool vtkCodedEntry::SetValueSchemeMeaning(const std::vector<std::string>& codeValueSchemeMeaning)
{
  if (codeValueSchemeMeaning.size() < 2 || codeValueSchemeMeaning.size() > 3)
  {
    vtkErrorMacro("SetValueSchemeMeaning failed: codeValueSchemeMeaning must contain 2 or 3 strings");
    return false;
  }
  this->SetCodeValue(codeValueSchemeMeaning[0].c_str());
  this->SetCodingSchemeDesignator(codeValueSchemeMeaning[1].c_str());
  if (codeValueSchemeMeaning.size() > 2)
  {
    this->SetCodeMeaning(codeValueSchemeMeaning[3].c_str());
  }
  else
  {
    this->SetCodeMeaning(codeValueSchemeMeaning[0].c_str());
  }
  return true;
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkCodedEntry::GetValueSchemeMeaning()
{
  std::vector<std::string> code;
  code.push_back(this->GetCodeValue() ? this->GetCodeValue() : "");
  code.push_back(this->GetCodingSchemeDesignator() ? this->GetCodingSchemeDesignator() : "");
  code.push_back(this->GetCodeMeaning() ? this->GetCodeMeaning() : "");
  return code;
}

//----------------------------------------------------------------------------
std::string vtkCodedEntry::GetAsPrintableString()
{
  std::string printable = std::string("(")                                                                    //
                          + (this->CodeValue ? this->CodeValue : "(none)") + ", "                             //
                          + (this->CodingSchemeDesignator ? this->CodingSchemeDesignator : "(none)") + ", \"" //
                          + (this->CodeMeaning ? this->CodeMeaning : "") + "\")";
  return printable;
}

//----------------------------------------------------------------------------
std::string vtkCodedEntry::GetAsString()
{
  std::string str;
  if (this->CodeValue)
  {
    str += "CodeValue:";
    str += this->CodeValue;
  }
  if (this->CodingSchemeDesignator)
  {
    if (!str.empty())
    {
      str += "|";
    }
    str += "CodingSchemeDesignator:";
    str += this->CodingSchemeDesignator;
  }
  if (this->CodeMeaning)
  {
    if (!str.empty())
    {
      str += "|";
    }
    str += "CodeMeaning:";
    str += this->CodeMeaning;
  }
  return str;
}

//----------------------------------------------------------------------------
bool vtkCodedEntry::SetFromString(const std::string& content)
{
  this->Initialize();
  bool success = true;
  std::stringstream attributes(content);
  std::string attribute;
  while (std::getline(attributes, attribute, '|'))
  {
    int colonIndex = attribute.find(':');
    std::string name = attribute.substr(0, colonIndex);
    std::string value = attribute.substr(colonIndex + 1);
    if (name == "CodeValue")
    {
      this->SetCodeValue(value.c_str());
    }
    else if (name == "CodingSchemeDesignator")
    {
      this->SetCodingSchemeDesignator(value.c_str());
    }
    else if (name == "CodeMeaning")
    {
      this->SetCodeMeaning(value.c_str());
    }
    else
    {
      vtkWarningMacro("Parsing coded entry string failed: unknown name " << name << " in " + content);
      success = false;
    }
  }
  if (this->GetCodeValue() == nullptr)
  {
    vtkWarningMacro("Parsing coded entry string failed: CodeValue is not specified in " + content);
    success = false;
  }
  if (this->GetCodingSchemeDesignator() == nullptr)
  {
    vtkWarningMacro("Parsing coded entry string failed: CodingSchemeDesignator is not specified in " + content);
    success = false;
  }
  // CodeMeaning is optional
  return success;
}

//----------------------------------------------------------------------------
bool vtkCodedEntry::AreEqual(vtkCodedEntry* entry1, vtkCodedEntry* entry2)
{
  if (entry1 == entry2)
  {
    // they are the same object or both nullptr
    return true;
  }
  if (!entry1 || !entry2)
  {
    // only one is nullptr
    return false;
  }
  // Neither entry is nullptr
  if ((entry1->GetCodeValue() == nullptr) != (entry2->GetCodeValue() == nullptr))
  {
    // Only one of them is nullptr
    return false;
  }
  if (entry2->GetCodeValue() && entry1->GetCodeValue() && strcmp(entry2->GetCodeValue(), entry1->GetCodeValue()) != 0)
  {
    return false;
  }
  if ((entry1->GetCodingSchemeDesignator() == nullptr) != (entry2->GetCodingSchemeDesignator() == nullptr))
  {
    // Only one of them is nullptr
    return false;
  }
  if (entry2->GetCodingSchemeDesignator() && entry1->GetCodingSchemeDesignator() //
      && strcmp(entry2->GetCodingSchemeDesignator(), entry1->GetCodingSchemeDesignator()) != 0)
  {
    return false;
  }
  if ((entry1->GetCodeMeaning() == nullptr) != (entry2->GetCodeMeaning() == nullptr))
  {
    // Only one of them is nullptr
    return false;
  }
  if (entry2->GetCodeMeaning() && entry1->GetCodeMeaning() && strcmp(entry2->GetCodeMeaning(), entry1->GetCodeMeaning()) != 0)
  {
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkCodedEntry::IsEmpty()
{
  if (this->CodeValue && strlen(this->CodeValue) > 0)
  {
    return false;
  }
  if (this->CodingSchemeDesignator && strlen(this->CodingSchemeDesignator) > 0)
  {
    return false;
  }
  if (this->CodeMeaning && strlen(this->CodeMeaning) > 0)
  {
    return false;
  }
  return true;
}
