#include <UIKit/UIDevice.h> // importing this means we have to set our app to be Objective-C++, sorry!
#include "cinder/app/AppCocoaTouch.h"
#include "cinder/app/Renderer.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/TextureFont.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// map our own shorter constants to Apple's long ones :)
enum Orientation { 
    PORTRAIT = UIDeviceOrientationPortrait, 
    LANDSCAPE_LEFT = UIDeviceOrientationLandscapeLeft, 
    UPSIDE_DOWN_PORTRAIT = UIDeviceOrientationPortraitUpsideDown, 
    LANDSCAPE_RIGHT = UIDeviceOrientationLandscapeRight 
    // note that Apple has face-up, face-down and unknown too, but we filter these out in our callback
};

// declare this function outside of the Cinder App so that Objective-C blocks can be used
void setupNotifications( class iPhoneOrientationApp *app );

class iPhoneOrientationApp : public AppCocoaTouch {
  public:
    
	virtual void	setup();
	virtual void	update();
	virtual void	draw();
    
    void            setDeviceOrientation( Orientation orientation, bool animate = true );

  private:

   	void        drawDevice();
	void        drawInterface();
    
    string      getOrientationDescription( Orientation orientation );
    float       getDeviceAngle( Orientation orientation );
        
    Vec2f       deviceSize;           // constant
    Orientation deviceOrientation;    // updated via setupNotifications 

    Vec2f       targetInterfaceSize;  // depends on deviceOrientation
    float       targetInterfaceAngle; // normalized for shortest rotation animation
    
    Vec2f       interfaceSize;        // animated, based on target
    Orientation interfaceOrientation; // depends on deviceOrientation
    float       interfaceAngle;       // animated, not always right-angle
    Matrix44f   orientationMatrix;    // for convenient inverse, etc.
    
    Vec2f       prevInterfaceSize;    // previous size, for animating
    float       prevInterfaceAngle;   // previous angle, for animating
    float       orientationChangeTime;// change time, for animating
    
    gl::TextureFontRef mTextureFont;  // new hotness for drawing dynamic text
};

void iPhoneOrientationApp::setup()
{
    // this is the size of our simulated "device" on screen:
    deviceSize = Vec2f(384, 512);
    
    // adds observer and initializes deviceOrientation
    setupNotifications(this);
    
    // this is the setup of the interface inside our simulated device
    interfaceSize = deviceSize;
    interfaceOrientation = deviceOrientation;
    interfaceAngle = 0.0;
    
    // initially we don't want to animate, so set these
    targetInterfaceSize = interfaceSize;
    targetInterfaceAngle = interfaceAngle;    
    prevInterfaceSize = interfaceSize;    
    prevInterfaceAngle = interfaceAngle;
    
    mTextureFont = gl::TextureFont::create( Font( "Helvetica-Bold", 24 ) );
    
}

// not a member function, so that blocks work correctly
void setupNotifications( iPhoneOrientationApp *app )
{
    [[NSNotificationCenter defaultCenter] addObserverForName:@"UIDeviceOrientationDidChangeNotification"
                                                      object:nil 
                                                       queue:nil 
                                                  usingBlock: ^(NSNotification *notification) {
        UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
        if (UIDeviceOrientationIsValidInterfaceOrientation(orientation)) {
            app->setDeviceOrientation(Orientation(orientation));
            // let's always make sure the task bar is shown on the correct side of the device            
            [UIApplication sharedApplication].statusBarOrientation = UIInterfaceOrientation(orientation);
        }
    }];
    // use initial taskbar orientation to derive a valid interface orientation
    app->setDeviceOrientation( Orientation([UIApplication sharedApplication].statusBarOrientation), false );
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];            
}

void iPhoneOrientationApp::setDeviceOrientation( Orientation orientation, bool animate )
{
    deviceOrientation = orientation;

    if (interfaceOrientation != deviceOrientation) {
        
        // update interface orientation
        interfaceOrientation = deviceOrientation;
        
        // normalize interfaceAngle (could be many turns)
        while (interfaceAngle < 0.0) interfaceAngle += 2.0f * M_PI;
        while (interfaceAngle > 2.0f * M_PI) interfaceAngle -= 2.0f * M_PI;
            
        // assign new targets
        float orientationAngle = getDeviceAngle(deviceOrientation);
        targetInterfaceSize.x = fabs(deviceSize.x * cos(orientationAngle) + deviceSize.y * sin(orientationAngle));
        targetInterfaceSize.y = fabs(deviceSize.y * cos(orientationAngle) + deviceSize.x * sin(orientationAngle));
        targetInterfaceAngle = 2.0f*M_PI-orientationAngle;
        
        // make sure we're turning the right way
        if (abs(targetInterfaceAngle-interfaceAngle) > M_PI) {
            if (targetInterfaceAngle < interfaceAngle) {
                targetInterfaceAngle += 2.0f * M_PI;
            }
            else {
                targetInterfaceAngle -= 2.0f * M_PI;
            }
        }
        
        // remember previous settings for animating
        prevInterfaceSize = interfaceSize;
        prevInterfaceAngle = interfaceAngle;

        if (animate) {
            orientationChangeTime = getElapsedSeconds();
        }
        else {
            orientationChangeTime = -1.0f;
        }
    }    
}

void iPhoneOrientationApp::update()
{
    float timeSinceChange = getElapsedSeconds() - orientationChangeTime;
    float animationDuration = 0.25f;
    
    // animate transition
    if (timeSinceChange < animationDuration) {
        float p = timeSinceChange / animationDuration;
        interfaceSize = lerp( prevInterfaceSize, targetInterfaceSize, p );
        interfaceAngle = lerp( prevInterfaceAngle, targetInterfaceAngle, p );
    } else {
        interfaceSize = targetInterfaceSize;
        interfaceAngle = targetInterfaceAngle;        
    }

    // apply transition
    orientationMatrix.setToIdentity();
    orientationMatrix.translate( Vec3f( deviceSize / 2.0f, 0 ) );
    orientationMatrix.rotate( Vec3f( 0, 0, interfaceAngle ) );
    orientationMatrix.translate( Vec3f( -interfaceSize / 2.0f, 0 ) );
}

void iPhoneOrientationApp::draw()
{
    gl::clear( Color::white() );
    gl::setMatricesWindow( getWindowSize() );
    
    gl::enableAlphaBlending();
    
    glPushMatrix();
    gl::translate( (getWindowSize() - deviceSize)/2.0f );
    drawDevice();
    
    glPushMatrix();
    glMultMatrixf(orientationMatrix);
    drawInterface();
    glPopMatrix();
    
    glPopMatrix();
}

void iPhoneOrientationApp::drawDevice()
{
    gl::color( Color::black() );
    gl::drawStrokedRect( Rectf( Vec2f(-10.0f,-10.0f), deviceSize+Vec2f(10.0f,40.0f) ) );
    gl::drawStrokedRect( Rectf( Vec2f(0.0f,0.0f), deviceSize ) );
    
    gl::drawStrokedCircle( Vec2f(deviceSize.x/2.0f, deviceSize.y+20.0f), 12.0f );
    gl::drawStrokedRect( Rectf( Vec2f(deviceSize.x/2.0f - 5.0f, deviceSize.y+20.0f-5.0f), Vec2f(deviceSize.x/2.0f + 5.0f, deviceSize.y+20.0f+5.0f) ) );
    
    mTextureFont->drawString("Device: " + getOrientationDescription(deviceOrientation), Vec2f( 0.0f, deviceSize.y+50.0f+mTextureFont->getFont().getAscent() ) );    
}

void iPhoneOrientationApp::drawInterface()
{
    gl::color( ColorA( 0.5f, 0.5f, 0.5f, 0.5f ) );
    gl::drawSolidRect( Rectf( Vec2f(0, 0), interfaceSize ) ); 

//    gl::drawSolidRect( Rectf( Vec2f(0, 0), Vec2f(interfaceSize.x,50.0f) ) ); 
//    gl::drawSolidRect( Rectf( Vec2f(0, interfaceSize.y-50.0f), interfaceSize ) ); 
    
    gl::color( Color::black() );
    string text = "Interface:\n"+getOrientationDescription( interfaceOrientation );
    Vec2f textSize = mTextureFont->measureString( text );
    mTextureFont->drawString( text, (interfaceSize - textSize)*0.5f );
}

string iPhoneOrientationApp::getOrientationDescription(Orientation orientation)
{
    switch (orientation) {
        case PORTRAIT:
            return "Portrait";
        case LANDSCAPE_LEFT:
            return "Landscape Left";
        case UPSIDE_DOWN_PORTRAIT:
            return "Upside Down Portrait";
        case LANDSCAPE_RIGHT:
            return "Landscape Right";
    }
    return "Unknown Orientation";
}

float iPhoneOrientationApp::getDeviceAngle(Orientation orientation)
{
    switch (orientation) {
        case PORTRAIT:
            return 0.0;
        case LANDSCAPE_LEFT:
            return M_PI * 1.5f;
        case UPSIDE_DOWN_PORTRAIT:
            return M_PI;
        case LANDSCAPE_RIGHT:
            return M_PI * 0.5f;
    }
    return 0.0;
}

CINDER_APP_COCOA_TOUCH( iPhoneOrientationApp, RendererGl )
