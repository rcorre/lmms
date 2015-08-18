/*
 * ExportProjectDialog.cpp - implementation of dialog for exporting project
 *
 * Copyright (c) 2004-2013 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

#include "ExportProjectDialog.h"
#include "Song.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "BBTrackContainer.h"
#include "BBTrack.h"


ExportProjectDialog::ExportProjectDialog( const QString & _file_name,
							QWidget * _parent, bool multi_export=false ) :
	QDialog( _parent ),
	Ui::ExportProjectDialog(),
	m_fileName( _file_name ),
	m_fileExtension(),
	m_multiExport( multi_export ),
	m_activeRenderer( NULL )
{
	setupUi( this );
	setWindowTitle( tr( "Export project to %1" ).arg( 
					QFileInfo( _file_name ).fileName() ) );

	// get the extension of the chosen file
	QStringList parts = _file_name.split( '.' );
	QString fileExt;
	if( parts.size() > 0 )
	{
		fileExt = "." + parts[parts.size()-1];
	}

	int cbIndex = 0;
	for( int i = 0; i < ProjectRenderer::NumFileFormats; ++i )
	{
		if( __fileEncodeDevices[i].m_getDevInst != NULL )
		{
			// get the extension of this format
			QString renderExt = __fileEncodeDevices[i].m_extension;

			// add to combo box
			fileFormatCB->addItem( ProjectRenderer::tr(
				__fileEncodeDevices[i].m_description ) );

			// if this is our extension, select it
			if( QString::compare( renderExt, fileExt,
									Qt::CaseInsensitive ) == 0 )
			{
				fileFormatCB->setCurrentIndex( cbIndex );
			}

			cbIndex++;
		}
	}

	connect( startButton, SIGNAL( clicked() ),
			this, SLOT( startBtnClicked() ) );

}




ExportProjectDialog::~ExportProjectDialog()
{
	delete m_activeRenderer;
	delete m_multiRenderer;
}




void ExportProjectDialog::reject()
{
	if( m_activeRenderer ) {
		m_activeRenderer->abortProcessing();
	}

	QDialog::reject();
}



void ExportProjectDialog::closeEvent( QCloseEvent * _ce )
{
	if( m_activeRenderer && m_activeRenderer->isRunning() ) {
		m_activeRenderer->abortProcessing();
	}

	QDialog::closeEvent( _ce );
}


void ExportProjectDialog::renderTracks()
{
	Mixer::qualitySettings qs =
			Mixer::qualitySettings(
					static_cast<Mixer::qualitySettings::Interpolation>(interpolationCB->currentIndex()),
					static_cast<Mixer::qualitySettings::Oversampling>(oversamplingCB->currentIndex()) );

	const int samplerates[5] = { 44100, 48000, 88200, 96000, 192000 };
	const int bitrates[6] = { 64, 128, 160, 192, 256, 320 };

	ProjectRenderer::OutputSettings os = ProjectRenderer::OutputSettings(
			samplerates[ samplerateCB->currentIndex() ],
			false,
			bitrates[ bitrateCB->currentIndex() ],
			static_cast<ProjectRenderer::Depths>( depthCB->currentIndex() ) );

	Engine::getSong()->setExportLoop( exportLoopCB->isChecked() );
	Engine::getSong()->setRenderBetweenMarkers( renderMarkersCB->isChecked() );

	m_multiRenderer = new MultiRender( qs, os, m_ft, m_fileName );

	m_multiRenderer->start();
}



void ExportProjectDialog::renderProject()
{
	Mixer::qualitySettings qs =
			Mixer::qualitySettings(
					static_cast<Mixer::qualitySettings::Interpolation>(interpolationCB->currentIndex()),
					static_cast<Mixer::qualitySettings::Oversampling>(oversamplingCB->currentIndex()) );

	const int samplerates[5] = { 44100, 48000, 88200, 96000, 192000 };
	const int bitrates[6] = { 64, 128, 160, 192, 256, 320 };

	ProjectRenderer::OutputSettings os = ProjectRenderer::OutputSettings(
			samplerates[ samplerateCB->currentIndex() ],
			false,
			bitrates[ bitrateCB->currentIndex() ],
			static_cast<ProjectRenderer::Depths>( depthCB->currentIndex() ) );

	Engine::getSong()->setExportLoop( exportLoopCB->isChecked() );
	Engine::getSong()->setRenderBetweenMarkers( renderMarkersCB->isChecked() );

	auto renderer = new ProjectRenderer( qs, os, m_ft, m_fileName );

	if( renderer->isReady() )
	{
		connect( renderer, SIGNAL( progressChanged( int ) ), progressBar, SLOT( setValue( int ) ) );
		connect( renderer, SIGNAL( progressChanged( int ) ), this, SLOT( updateTitleBar( int ) )) ;
		connect( renderer, SIGNAL( finished() ), this, SLOT( accept() ) );
		connect( renderer, SIGNAL( finished() ), gui->mainWindow(), SLOT( resetWindowTitle() ) );

		m_activeRenderer = renderer;
		renderer->startProcessing();
	}
	else
	{
		accept();
	}
}



void ExportProjectDialog::startBtnClicked()
{
	m_ft = ProjectRenderer::NumFileFormats;

	for( int i = 0; i < ProjectRenderer::NumFileFormats; ++i )
	{
		if( fileFormatCB->currentText() ==
			ProjectRenderer::tr(
				__fileEncodeDevices[i].m_description ) )
		{
			m_ft = __fileEncodeDevices[i].m_fileFormat;
			m_fileExtension = QString( QLatin1String( __fileEncodeDevices[i].m_extension ) );
			break;
		}
	}

	if( m_ft == ProjectRenderer::NumFileFormats )
	{
		QMessageBox::information( this, tr( "Error" ),
			tr( "Error while determining file-encoder device. "
				"Please try to choose a different output "
							"format." ) );
		reject();
		return;
	}

	startButton->setEnabled( false );
	progressBar->setEnabled( true );

	updateTitleBar( 0 );

	if (m_multiExport==true)
	{
		renderTracks();
	}
	else
	{
		renderProject();
	}
}




void ExportProjectDialog::updateTitleBar( int _prog )
{
	gui->mainWindow()->setWindowTitle(
					tr( "Rendering: %1%" ).arg( _prog ) );
}
