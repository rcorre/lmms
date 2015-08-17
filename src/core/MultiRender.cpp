/*
 * MultiRender.cpp - implementation of dialog for exporting project
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

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <stdio.h>

#include "MultiRender.h"
#include "Song.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "BBTrackContainer.h"
#include "BBTrack.h"


MultiRender::MultiRender( const Mixer::qualitySettings & _qs,
			const ProjectRenderer::OutputSettings & _os,
			ProjectRenderer::ExportFileFormats & _file_format,
			const QString & _output_dir ) :
	m_qualitySettings( _qs ),
	m_outputSettings( _os ),
	m_ft( _file_format ),
	m_outputDir( _output_dir ),
	m_fileExtension( ProjectRenderer::getFileExtensionFromFormat( _file_format ) ),
	m_activeRenderer( NULL )
{
}

MultiRender::~MultiRender()
{
}

void MultiRender::start()
{
	if( m_ft == ProjectRenderer::NumFileFormats )
	{
		return; // we were not given a file format, give up
	}

	const TrackContainer::TrackList & tl = Engine::getSong()->tracks();
	// Check for all unmuted tracks. Remember list.
	for( TrackContainer::TrackList::ConstIterator it = tl.begin();
							it != tl.end(); ++it )
	{
		Track* tk = (*it);
		Track::TrackTypes type = tk->type();
		// Don't mute automation tracks
		if ( tk->isMuted() == false &&
				( type == Track::InstrumentTrack || type == Track::SampleTrack ) )
		{
			m_tracksToRender.push_back(tk);
		}
	}

	const TrackContainer::TrackList t2 = Engine::getBBTrackContainer()->tracks();
	for( TrackContainer::TrackList::ConstIterator it = t2.begin(); it != t2.end(); ++it )
	{
		Track* tk = (*it);
		if ( tk->isMuted() == false )
		{
			m_tracksToRender.push_back(tk);
		}
	}

	qDebug() << "starting render";
	int trackNum = 1;
	for( TrackContainer::TrackList::ConstIterator it = m_tracksToRender.begin();
			 it != m_tracksToRender.end();
			 ++it )
	{
		qDebug() << "rendering " << trackNum;
		renderTrack(*it, trackNum++);
	}
}

void MultiRender::updateConsoleProgress()
{
	m_activeRenderer->updateConsoleProgress();
}

// Render one of the tracks from the song.
void MultiRender::renderTrack(const Track * track, int trackNum)
{
	// determine the path this track will be written to
	QString trackName = track->name();
	trackName = trackName.remove(QRegExp("[^a-zA-Z]"));
	trackName = QString( "%1_%2%3" ).arg( trackNum ).arg( trackName ).arg( m_fileExtension );
	QString outPath = QDir(m_outputDir).filePath(trackName);

	// mute every track but the one we are about to render
	for( TrackVector::ConstIterator it = m_tracksToRender.begin();
			 it != m_tracksToRender.end();
			 ++it )
	{
		(*it)->setMuted( (*it) == track );
	}

	// create a renderer for this track
	m_activeRenderer = new ProjectRenderer( m_qualitySettings,
	                                        m_outputSettings,
	                                        m_ft,
	                                        outPath );

	// connect( renderer, SIGNAL( finished() ), this, SLOT( accept() ) );
	m_activeRenderer->startProcessing();
	m_activeRenderer->wait();
	delete m_activeRenderer;
	m_activeRenderer = NULL;
}
