#include "StdAfx.h"
#include "DownloadComponent.h"

using namespace DVLib;

DownloadComponent::DownloadComponent(
	IDownloadCallback * p_Callback, 
	DownloadComponentInfo * p_Component, 
	int p_CurrentComponent, 
	int p_TotalComponents)
{
	m_bCanceledByTheUser = false;
	m_Callback = p_Callback;
	m_Component = p_Component;
	m_CurrentComponent = p_CurrentComponent;
	m_TotalComponents = p_TotalComponents;
}

bool DownloadComponent::IsDownloadRequired() const
{
	CString l_destinationFullFileName = GetDestinationFileName();
	BOOL fileExists = FileExistsCustom(l_destinationFullFileName);
	return (m_Component->AlwaysDownload || ! fileExists);
}

CString DownloadComponent::GetDestinationFileName() const
{
	if (m_Component->DestinationFileName.GetLength() <= 0)
	{
		return DVLib::PathCombineCustom(
			m_Component->DestinationPath, 
			DVLib::GetFileNameFromFullFilePath(m_Component->SourceURL));
	}
	else
	{
		return DVLib::PathCombineCustom(
			m_Component->DestinationPath, 
			m_Component->DestinationFileName);
	}
}

void DownloadComponent::StartDownload()
{
	CString l_destinationFullFileName = GetDestinationFileName();

	ApplicationLog.Write( TEXT("SourcePath: "), m_Component->SourcePath);
	ApplicationLog.Write( TEXT("DestinationPath: "), m_Component->DestinationPath);
	CreateDirectory(m_Component->DestinationPath,NULL); //cerco comunque di creare la directory
	
	ApplicationLog.Write( TEXT("DestinationFullFileName: "), l_destinationFullFileName);
	ApplicationLog.Write( TEXT("AlwaysDownload: "), m_Component->AlwaysDownload ? TEXT("True") : TEXT("False") );
	ApplicationLog.Write( TEXT("FileExists: "), FileExistsCustom(l_destinationFullFileName) ? TEXT("True") : TEXT("False") );

	if (! IsDownloadRequired())
	{
		ApplicationLog.Write( TEXT("Skipping download.") );
		return;
	}

	if (! m_Component->SourcePath.IsEmpty() && FileExistsCustom(m_Component->SourcePath))
	{
		ApplicationLog.Write( TEXT("Copying: "), m_Component->SourcePath);
		if (! CopyFile(m_Component->SourcePath, l_destinationFullFileName, false))
		{
			std::string error = "Error copying ";
			error.append(DVLib::Tstring2string(m_Component->SourcePath));
			error.append("\r\n");
			error.append(DVLib::Tstring2string(DVLib::TranslateErrorMsg(DVLib::LastError())));
			throw std::exception(error.c_str());
		}
	}
	else
	{
		ApplicationLog.Write( TEXT("Downloading: "), m_Component->SourceURL);
		HRESULT l_hrRet = URLDownloadToFile(NULL, m_Component->SourceURL, l_destinationFullFileName, 0, this);
		if (FAILED(l_hrRet))
		{
			std::string error = "Error downloading ";
			error.append(DVLib::Tstring2string(m_Component->SourceURL));
			error.append("\r\n");
			error.append(DVLib::Tstring2string(DVLib::TranslateErrorMsg(l_hrRet)));
			throw std::exception(error.c_str());
		}
	}
}

UINT DownloadComponents(IDownloadCallback * p_Callback)
{
	CString l_tmpLastComponent = TEXT("-");
	try
	{
		DownloadComponentInfoVector * l_List = p_Callback->GetComponents();
		for (int i = 0; i < l_List->GetCount(); i++)
		{
			//verifico se l'utente ha premuto cancel
			if (p_Callback->WantToStop())
			{
				p_Callback->CanceledByTheUser();
				return 0;
			}

			l_tmpLastComponent = l_List->GetAt(i).ComponentName;

			//Download del componente
			DownloadComponent l_Component(p_Callback, &(l_List->GetAt(i)), i+1, (int)l_List->GetCount() );
			l_Component.StartDownload();


			//verifico se l'utente ha premuto cancel
			if (l_Component.IsCanceledByTheUser())
			{
				p_Callback->CanceledByTheUser();
				return 0;
			}
		}

		p_Callback->DownloadComplete();
	}
	catch(std::exception& ex)
	{
	    p_Callback->DownloadError(DVLib::string2Tstring(ex.what()).c_str());
		return 0;
	}

	return 0;
}