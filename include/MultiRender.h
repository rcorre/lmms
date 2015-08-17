/*
 * MultiRender.h - declaration of class MultiRender which is responsible for
 *                 exporting tracks of a song individually.
 *
 * Copyright (c) 2004-2012 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef EXPORT_MULTI_RENDER_H
#define EXPORT_MULTI_RENDER_H

#include <vector>

#include "ProjectRenderer.h"


class MultiRender : public QObject
{
  Q_OBJECT
public:
	MultiRender( const Mixer::qualitySettings & _qs,
			const ProjectRenderer::OutputSettings & _os,
			ProjectRenderer::ExportFileFormats & _file_format,
			const QString & _dir_path );

	virtual ~MultiRender();

	void start();

private slots:
	void updateConsoleProgress();
	void renderNextTrack();

private:
	void renderTrack(Track *track);

	const Mixer::qualitySettings & m_qualitySettings;
	const ProjectRenderer::OutputSettings & m_outputSettings;
	ProjectRenderer::ExportFileFormats m_ft;
	QString m_outputDir;
	QString m_fileExtension;
	ProjectRenderer* m_activeRenderer;

	typedef QVector<Track*> TrackVector;
	TrackVector m_tracksToRender;
	int m_trackNum;
	int m_numTracks;
} ;

#endif
