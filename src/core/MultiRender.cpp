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
	m_activeRenderer( NULL ),
	m_trackNum( 1 )
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

	// Check for all unmuted tracks -- we will render these.
	// Start by muting all of them, they will later be unmuted one at a time.
	const TrackContainer::TrackList & tl = Engine::getSong()->tracks();
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
			tk->setMuted( true );
		}
	}

	const TrackContainer::TrackList t2 = Engine::getBBTrackContainer()->tracks();
	for( TrackContainer::TrackList::ConstIterator it = t2.begin(); it != t2.end(); ++it )
	{
		Track* tk = (*it);
		if ( tk->isMuted() == false )
		{
			m_tracksToRender.push_back(tk);
			tk->setMuted( true );
		}
	}

	m_numTracks = m_tracksToRender.size();

	renderTrack(m_tracksToRender.back());
}

void MultiRender::updateConsoleProgress()
{
	m_activeRenderer->updateConsoleProgress();
	fprintf( stderr, "(%d/%d) ", m_trackNum, m_numTracks );
}

// Unmute and render the given track.
void MultiRender::renderTrack(Track *track)
{
	track->setMuted( false );

	// determine the path this track will be written to
	QString trackName = track->name();
	trackName = trackName.remove(QRegExp("[^a-zA-Z]"));
	trackName = QString( "%1_%2%3" )
		.arg( m_trackNum ).arg( trackName ).arg( m_fileExtension );
	QString outPath = QDir(m_outputDir).filePath(trackName);

	// create a renderer for this track
	m_activeRenderer = new ProjectRenderer( m_qualitySettings,
	                                        m_outputSettings,
	                                        m_ft,
	                                        outPath );

	connect( m_activeRenderer, SIGNAL( finished() ), this,
			SLOT( renderNextTrack() ) );

	m_activeRenderer->startProcessing();
}

// Pop the current track and render the next track.
void MultiRender::renderNextTrack()
{
	// mute and pop the track we just finished
	m_tracksToRender.back()->setMuted( true );
	m_tracksToRender.pop_back();

	delete m_activeRenderer;
	++m_trackNum;

	if ( !m_tracksToRender.isEmpty() )
	{
		// more tracks left to render
		renderTrack(m_tracksToRender.back());
	}
}
