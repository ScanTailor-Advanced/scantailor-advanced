// IosDirectoryPicker.mm
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "IosDirectoryPicker.h"
#include <QString>
#include <functional>

@interface STDirectoryPickerDelegate : NSObject <UIDocumentPickerDelegate>
@property (nonatomic, copy) void (^completion)(NSURL* url);
@end

@implementation STDirectoryPickerDelegate
- (void)documentPicker:(UIDocumentPickerViewController*)controller
  didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
  if (self.completion) {
    self.completion(urls.firstObject);
    self.completion = nil;
  }
}
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller {
  if (self.completion) {
    self.completion(nil);
    self.completion = nil;
  }
}
@end

static UIViewController* topViewController() {
  UIViewController* root = nil;
  for (UIScene* scene in [UIApplication sharedApplication].connectedScenes) {
    if ([scene isKindOfClass:[UIWindowScene class]]) {
      UIWindowScene* ws = (UIWindowScene*)scene;
      for (UIWindow* w in ws.windows) {
        if (w.isKeyWindow) { root = w.rootViewController; break; }
      }
    }
  }
  while (root.presentedViewController) root = root.presentedViewController;
  return root;
}

// Keep delegate alive until picker is dismissed
static STDirectoryPickerDelegate* s_delegate = nil;

void iosPickDirectory(const QString& title,
                      std::function<void(const QString&)> callback) {
  auto* cb = new std::function<void(const QString&)>(std::move(callback));

  dispatch_async(dispatch_get_main_queue(), ^{
    UIDocumentPickerViewController* picker =
      [[UIDocumentPickerViewController alloc]
        initForOpeningContentTypes:@[
          UTTypeImage, UTTypeJPEG, UTTypePNG, UTTypeTIFF,
          UTTypeData, UTTypeFolder
        ]];
    picker.allowsMultipleSelection = NO;

    s_delegate = [[STDirectoryPickerDelegate alloc] init];
    s_delegate.completion = ^(NSURL* url) {
      QString path;
      if (url) {
        [url startAccessingSecurityScopedResource];
        NSNumber* isDir = nil;
        [url getResourceValue:&isDir forKey:NSURLIsDirectoryKey error:nil];
        NSURL* dir = isDir.boolValue ? url : [url URLByDeletingLastPathComponent];
        path = QString::fromNSString(dir.path);
      }
      (*cb)(path);
      delete cb;
      s_delegate = nil;
    };
    picker.delegate = s_delegate;

    UIViewController* top = topViewController();
    if (top) {
      [top presentViewController:picker animated:YES completion:nil];
    } else {
      (*cb)(QString());
      delete cb;
    }
  });
}
