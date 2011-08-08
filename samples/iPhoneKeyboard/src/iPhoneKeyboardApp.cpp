#import <UIKit/UIKit.h>

#include "cinder/app/AppCocoaTouch.h"
#include "cinder/app/Renderer.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"

using namespace ci;
using namespace ci::app;

@interface TFDelegate : NSObject <UITextFieldDelegate> {
@public
    UITextField *activeTextField;
    AppCocoaTouch *mApp;
}
@end

@implementation TFDelegate

- (BOOL)textFieldShouldBeginEditing:(UITextField *)textField
{
    activeTextField = textField;
    NSLog(@"textFieldShouldBeginEditing: sender = %d, %@",  [textField tag], [textField text]);
    return YES;
}

- (BOOL) textField: (UITextField *) textField shouldChangeCharactersInRange: (NSRange) range replacementString: (NSString *) string
{
    for (int i = 0; i < string.length; i++) {
        char c		= [string characterAtIndex: i];    
        int code	= 0; // TODO
        int mods	= 0; // TODO
        cinder::app::KeyEvent k = cinder::app::KeyEvent (cinder::app::KeyEvent::translateNativeKeyCode( code ), c, mods, code);	
        mApp->privateKeyDown__( k );
        mApp->privateKeyUp__( k );
    }
    return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
    NSLog(@"textFieldDidEndEditing: sender = %d, %@",  [textField tag], [textField text]);
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    NSLog(@"textFieldShouldReturn: sender = %d, %@",  [textField tag], [textField text]);
    activeTextField = nil;
    [textField resignFirstResponder];
    return YES;
}

@end


class iPhoneKeyboardApp : public AppCocoaTouch {
  public:
	virtual void	setup();
	virtual void	resize( ResizeEvent event );
	virtual void	update();
	virtual void	draw();
	virtual void	keyDown( KeyEvent event );
	virtual void	keyUp( KeyEvent event );
		
	Matrix44f	mCubeRotation;
	gl::Texture mTex;
	CameraPersp	mCam;
    
    TFDelegate *tfDelegate;
};

void iPhoneKeyboardApp::setup()
{
	mCubeRotation.setToIdentity();

	// Create a blue-green gradient as an OpenGL texture
	Surface8u surface( 256, 256, false );
	Surface8u::Iter iter = surface.getIter();
	while( iter.line() ) {
		while( iter.pixel() ) {
			iter.r() = 0;
			iter.g() = iter.x();
			iter.b() = iter.y();
		}
	}
	
	mTex = gl::Texture( surface );
    
    tfDelegate = [[TFDelegate alloc] init];
    tfDelegate->mApp = this; // to send keyup/down events
    
    UIWindow *window = [[UIApplication sharedApplication] keyWindow];
    
    UITextField *textField = [[UITextField alloc] initWithFrame:CGRectMake(25, 35, 300, 31)];	
    textField.borderStyle = UITextBorderStyleRoundedRect; // or maybe UISearchBar instead?
    textField.delegate = tfDelegate;
    textField.keyboardType = UIKeyboardTypeDefault;
    textField.returnKeyType = UIReturnKeyDone; 
    textField.textAlignment = UITextAlignmentLeft;	
    textField.placeholder = @"Username\n";	
    textField.autocorrectionType = UITextAutocorrectionTypeNo;
    textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    
 	[window addSubview: textField];

}

void iPhoneKeyboardApp::resize( ResizeEvent event )
{
	mCam.lookAt( Vec3f( 3, 2, -3 ), Vec3f::zero() );
	mCam.setPerspective( 60, event.getAspectRatio(), 1, 1000 );
}

void iPhoneKeyboardApp::keyDown( KeyEvent event )
{
    console() << "Key down: " << event.getChar() << std::endl;
}

void iPhoneKeyboardApp::keyUp( KeyEvent event )
{
    console() << "Key up: " << event.getChar() << std::endl;
}

void iPhoneKeyboardApp::update()
{
	mCubeRotation.rotate( Vec3f( 1, 1, 1 ), 0.03f );
}

void iPhoneKeyboardApp::draw()
{
	gl::clear( Color( 0.2f, 0.2f, 0.3f ) );
	gl::enable( GL_TEXTURE_2D );
	gl::enableDepthRead();
	
	mTex.bind();
	gl::setMatrices( mCam );
	glPushMatrix();
		gl::multModelView( mCubeRotation );
		gl::drawCube( Vec3f::zero(), Vec3f( 2.0f, 2.0f, 2.0f ) );
	glPopMatrix();
}

CINDER_APP_COCOA_TOUCH( iPhoneKeyboardApp, RendererGl )
