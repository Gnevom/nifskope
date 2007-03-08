/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools projectmay not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#include "uvedit.h"

#include "../nifmodel.h"
#include "../niftypes.h"
#include "../gl/options.h"
#include "../gl/gltex.h"
#include "../gl/gltools.h"

#include <math.h>
#include <gl/glext.h>

#define BASESIZE 512.0
#define GRIDSIZE 16.0
#define GRIDSEGS 4
#define ZOOMUNIT 64.0

static GLshort vertArray[4][2] = {
	{ 0, 0 }, { 1, 0 },
	{ 1, 1 }, { 0, 1 }
};

static GLshort texArray[4][2] = {
	{ 0, 1 }, { 1, 1 },
	{ 1, 0 }, { 0, 0 }
};

static GLdouble glUnit = ( 1.0 / BASESIZE );
static GLdouble glGridD = GRIDSIZE * glUnit;

UVWidget::UVWidget( QWidget * parent )
	: QGLWidget( QGLFormat( QGL::SampleBuffers ), parent )
{
	textures = new TexCache( this );

	pos.setX( 0 );
	pos.setY( 0 );

	zoom = 1.2;

	mousePos.setX( -1000 );
	mousePos.setY( -1000 );

	selectedType		= NoneSel;
	selectedTexCoord	= -1;
	selectedFace		= -1;
}

UVWidget::~UVWidget()
{
	delete textures;
}

void UVWidget::initializeGL()
{
	glMatrixMode( GL_MODELVIEW );

	initializeTextureUnits( context() );

	glShadeModel( GL_SMOOTH );
	glShadeModel( GL_LINE_SMOOTH );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glDepthRange( 0.0, -1.0 );

	glEnable( GL_MULTISAMPLE );
	glDisable( GL_LIGHTING );

	qglClearColor( GLOptions::bgColor() );

	bindTexture( texfile );
	
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_SHORT, 0, vertArray );

	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_SHORT, 0, vertArray );

	// check for errors
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR ) {
		qDebug() << "GL ERROR (init) : " << (const char *) gluErrorString( err );
	}
}

void UVWidget::resizeGL( int width, int height )
{
	updateViewRect( width, height );
}

void UVWidget::paintGL()
{
	makeCurrent();

	glPushAttrib( GL_ALL_ATTRIB_BITS );
	
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	setupViewport( width(), height() );

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glPushMatrix();
	glLoadIdentity();
	
	glEnable( GL_TEXTURE_2D );

	glTranslatef( -0.5f, -0.5f, 0.0f );

	glTranslatef( -1.0f, -1.0f, 0.0f );
	for( int i = 0; i < 3; i++ )
	{
		for( int j = 0; j < 3; j++ )
		{
			if( i != 1 || j != 1 ) {
				glColor4f( 1.0f, 0.8f, 0.8f, 1.0f );
			}

			glDrawArrays( GL_QUADS, 0, 4 );

			glTranslatef( 1.0f, 0.0f, 0.0f );
		}

		glTranslatef( -3.0f, 1.0f, 0.0f );
	}
	glTranslatef( 1.0f, -2.0f, 0.0f );
	
	glDisable( GL_TEXTURE_2D );
	
	glLineWidth( 2.0f );
	glColor4f( 0.5f, 0.5f, 0.5f, 0.5f );
	glBegin( GL_LINE_STRIP );
	for( int i = 0; i < 5; i++ )
	{
		glVertex2sv( vertArray[i % 4] );
	}
	glEnd();

	glPopMatrix();



	glPushMatrix();
	glLoadIdentity();

	glLineWidth( 0.8f );
	glBegin( GL_LINES );
	int glGridMinX	= qRound( glViewRect[0] / glGridD );
	int glGridMaxX	= qRound( glViewRect[1] / glGridD );
	int glGridMinY	= qRound( glViewRect[2] / glGridD );
	int glGridMaxY	= qRound( glViewRect[3] / glGridD );
	for( int i = glGridMinX; i < glGridMaxX; i++ )
	{
		GLdouble glGridPos = glGridD * i;

		if( ( i % GRIDSEGS ) == 0 ) {
			glLineWidth( 1.2f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
		}
		else if( zoom > 2.0 ) {
			continue;
		}

		glVertex2d( glGridPos, glViewRect[2] );
		glVertex2d( glGridPos, glViewRect[3] );

		if( ( i % GRIDSEGS ) == 0 ) {
			glLineWidth( 0.8f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
		}
	}
	for( int i = glGridMinY; i < glGridMaxY; i++ )
	{
		GLdouble glGridPos = glGridD * i;

		if( ( i % GRIDSEGS ) == 0 ) {
			glLineWidth( 1.2f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
		}
		else if( zoom > 2.0 ) {
			continue;
		}

		glVertex2d( glViewRect[0], glGridPos );
		glVertex2d( glViewRect[1], glGridPos );

		if( ( i % GRIDSEGS ) == 0 ) {
			glLineWidth( 0.8f );
			glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
		}
	}
	glEnd();

	glPopMatrix();



	drawTexCoords();



	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	glMatrixMode( GL_PROJECTION );
	glPopMatrix();

	glPopAttrib();
}

void UVWidget::drawTexCoords()
{
	QVector< int > selTexCoords( 1, selectedTexCoord );

	glMatrixMode( GL_MODELVIEW );

	glPushMatrix();
	glLoadIdentity();
	glTranslatef( -0.5f, -0.5f, 0.0f );

	Color4 nlColor( GLOptions::nlColor() );
	nlColor.setAlpha( 0.5f );
	Color4 hlColor( GLOptions::hlColor() );
	hlColor.setAlpha( 0.5f );

	glColor( nlColor );
	GLfloat zPos = 0.0f;
	GLint glName = texcoords.size() + faces.size();

	glLineWidth( 1.0f );
	for( int i = faces.size() - 1; i > -1; i-- )
	{
		if( selectedFace == i ) {
			glColor( hlColor );
			zPos = 0.1f;

			for( int j = 0; j < 3; j++ )
			{
				selTexCoords.append( faces[i].t[j] );
			}
		}

		glLoadName( glName );
		glBegin( GL_LINES );
		for( int j = 0; j < 3; j++ )
		{
			glVertex3f( texcoords[faces[i].t[j]][0], texcoords[faces[i].t[j]][1], zPos );
			glVertex3f( texcoords[faces[i].t[(j + 1) % 3]][0], texcoords[faces[i].t[(j + 1) % 3]][1], zPos );
		}
		glEnd();
		glName--;

		if( selectedFace == i ) {
			glColor( nlColor );
			zPos = 0.0f;
		}
	}

	glPointSize( 3.5f );
	for( int i = texcoords.size() - 1; i > -1; i-- )
	{
		bool selected = selTexCoords.contains( i );

		if( selected ) {
			glColor( hlColor );
			zPos = 0.1f;
		}

		glLoadName( glName );
		glBegin( GL_POINTS );
		glVertex2f( texcoords[i][0], texcoords[i][1] );
		glEnd();
		glName--;

		if( selected ) {
			glColor( nlColor );
			zPos = 0.0f;
		}
	}

	glPopMatrix();
}

void UVWidget::setupViewport( int width, int height )
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	glViewport( 0, 0, width, height );

	glOrtho( glViewRect[0], glViewRect[1], glViewRect[2], glViewRect[3], 0.0, 100.0 );
}

void UVWidget::updateViewRect( int width, int height )
{
	GLdouble glOffX	= glUnit * zoom * 0.5 * width;
	GLdouble glOffY	= glUnit * zoom * 0.5 * height;
	GLdouble glPosX = glUnit * zoom * pos.x();
	GLdouble glPosY = glUnit * zoom * pos.y();

	glViewRect[0] = - glOffX - glPosX;
	glViewRect[1] = + glOffX - glPosX;
	glViewRect[2] = - glOffY + glPosY;
	glViewRect[3] = + glOffY + glPosY;
}

int UVWidget::indexAt( const QPoint & hitPos )
{
	makeCurrent();

	glPushAttrib( GL_ALL_ATTRIB_BITS );

	GLuint buffer[512];
	glSelectBuffer( 512, buffer );

	glRenderMode( GL_SELECT );	

	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	glViewport( 0, 0, width(), height() );

	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	gluPickMatrix( hitPos.x(), viewport[3] - hitPos.y(), 8.0, 8.0, viewport );

	glOrtho( glViewRect[0], glViewRect[1], glViewRect[2], glViewRect[3], 0.0, 100.0 );
	
	glInitNames();
	glPushName( 0 );
	
	drawTexCoords();
	
	int selection = -1;
	GLint hits = glRenderMode( GL_RENDER );
	if( hits > 0 ) {
		QList< GLint > hitNames;
		for( int i = 0; i < hits; i++ )
		{
			hitNames.append( buffer[i * 4 + 3] );
		}
		qSort< QList< GLint > >( hitNames );
		selection = hitNames[selectCycle % hits];
	}

	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	
	glPopAttrib();

	return selection;
}

bool UVWidget::bindTexture( const QString & filename )
{
	GLuint mipmaps = 0;
	GLfloat max_anisotropy = 0.0f;

	QString extensions( (const char *) glGetString( GL_EXTENSIONS ) );
	
	if ( extensions.contains( "GL_EXT_texture_filter_anisotropic" ) )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, & max_anisotropy );
		//qWarning() << "maximum anisotropy" << max_anisotropy;
	}

	if ( mipmaps = textures->bind( filename ) )
	{
		if ( max_anisotropy > 0.0f )
		{
			if ( GLOptions::antialias() )
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy );
			else
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f );
		}

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

		// TODO: Add support for non-square textures

		glMatrixMode( GL_TEXTURE );
		glLoadIdentity();

		glMatrixMode( GL_MODELVIEW );
		return true;
	}
	
	return false;
 }



QSize UVWidget::sizeHint() const
{
	if( sHint.isValid() ) {
		return sHint;
	}

	return QSize( BASESIZE, BASESIZE );
}

void UVWidget::setSizeHint( const QSize & s )
{
	sHint = s;
}

QSize UVWidget::minimumSizeHint() const
{
	return QSize( BASESIZE, BASESIZE );
}

int UVWidget::heightForWidth( int width ) const
{
	if ( width < minimumSizeHint().height() )
		return minimumSizeHint().height();
	return width;
}

void UVWidget::mousePressEvent( QMouseEvent * e )
{
	QPoint dPos( e->pos() - mousePos );
	mousePos = e->pos();

	int selection;

	switch( e->button() ) {
		case Qt::LeftButton:
			if( dPos.manhattanLength() < 4 ) {
				selectCycle++;
			}
			else {
				selectCycle = 0;
			}

			selection = indexAt( e->pos() ) - 1;

			if( selection < 0 ) {
				selectedType = NoneSel;
				selectedTexCoord = -1;
				selectedFace = -1;
			}
			else if( selection < texcoords.size() ) {
				selectedType		= TexCoordSel;
				selectedTexCoord	= selection;
				selectedFace		= -1;
			}
			else {
				selectedType		= FaceSel;
				selectedTexCoord	= -1;
				selectedFace		= selection - texcoords.size();
			}

			break;

		default:
			return;
	}

	updateGL();
}

void UVWidget::mouseReleaseEvent( QMouseEvent * e )
{
}


void UVWidget::mouseMoveEvent( QMouseEvent * e )
{
	QPoint dPos( e->pos() - mousePos );

	switch( e->buttons()) {
		case Qt::LeftButton:
			switch( selectedType )
			{
				case TexCoordSel:
					texcoords[selectedTexCoord][0] += glUnit * zoom * dPos.x();
					texcoords[selectedTexCoord][1] -= glUnit * zoom * dPos.y();
					updateNif();
					break;

				default:
					return;
			}
			break;

		case Qt::MidButton:
			pos += dPos;
			updateViewRect( width(), height() );
			break;

		case Qt::RightButton:
			zoom *= 1.0 + ( dPos.y() / ZOOMUNIT );

			if( zoom < 0.1 ) {
				zoom = 0.1;
			}
			else if( zoom > 10.0 ) {
				zoom = 10.0;
			}

			updateViewRect( width(), height() );

			break;

		default:
			return;
	}

	mousePos = e->pos();

	updateGL();
}

void UVWidget::wheelEvent( QWheelEvent * e )
{
	zoom *= 1.0 + ( e->delta() / 8.0 ) / ZOOMUNIT;

	if( zoom < 0.1 ) {
		zoom = 0.1;
	}
	else if( zoom > 10.0 ) {
		zoom = 10.0;
	}

	updateViewRect( width(), height() );

	updateGL();
}



bool UVWidget::setNifData( NifModel * nifModel, const QModelIndex & nifIndex )
{
	nif = nifModel;
	idx = nifIndex;

	textures->setNifFolder( nif->getFolder() );

	QModelIndex iData = nif->getBlock( nif->getLink( idx, "Data" ) );
	if( nif->inherits( iData, "NiTriBasedGeomData" ) )
	{
		QModelIndex iUV = nif->getIndex( nif->getBlock( iData ), "UV Sets" );
		if( !iUV.isValid() ) {
			return false;
		}

		iTexCoords = iUV.child( 0, 0 );
		if( nif->isArray( iTexCoords ) ) {
			texcoords = nif->getArray< Vector2 >( iTexCoords );
		}
		else {
			return false;
		}
		
		if( nif->isNiBlock( idx, "NiTriShape" ) ) {
			QVector< Triangle > tris = nif->getArray< Triangle >( iData, "Triangles" );
			QMutableVectorIterator< Triangle > itri( tris );
			while ( itri.hasNext() )
			{
				Triangle & t = itri.next();
				face f;
				f.t[0] = t[0];
				f.t[1] = t[1];
				f.t[2] = t[2];

				faces.append( f );
			}
		}
		else if( nif->isNiBlock( idx, "NiTriStrips" ) ) {
			QModelIndex iPoints = nif->getIndex( nif->getBlock( iData ), "Points" );
			if( !iPoints.isValid() ) {
				return false;
			}

			QVector< ushort > pnts = nif->getArray< ushort >( iPoints, "Points" );
			for( int i = 0; i < ( pnts.size() - 2 ); i++ )
			{
				face f;
				f.t[0] = pnts[i];
				f.t[1] = pnts[i+1];
				f.t[2] = pnts[i+2];

				faces.append( f );
			}
		}
		else {
			return false;
		}
	}

	foreach( qint32 l, nif->getLinkArray( idx, "Properties" ) )
	{
		QModelIndex iProp = nif->getBlock( l, "NiProperty" );
		if( iProp.isValid() ) {
			if( nif->isNiBlock( iProp, "NiTexturingProperty" ) ) {
				foreach ( qint32 sl, nif->getChildLinks( nif->getBlockNumber( iProp ) ) )
				{
					QModelIndex iTexSource = nif->getBlock( sl, "NiSourceTexture" );
					if( iTexSource.isValid() )
					{
						texfile = TexCache::find( nif->get<QString>( iTexSource, "File Name" ) , nif->getFolder() );
						return true;
					}
				}
			}
		}
	}

	return false;
}

void UVWidget::updateNif()
{
	if( iTexCoords.isValid() ) {
		nif->setArray< Vector2 >( iTexCoords, texcoords );
	}
}