/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

import Foundation
import SnapKit
import UIKit

extension UIView {
	
	// Few constraints wrapper to abstract SnapKit functions
	
	func removeConstraints() -> UIView  {
		snp.removeConstraints()
		return self
	}
	
	func square(_ size:Int) -> UIView {
		snp.makeConstraints { (make) in
			make.width.equalTo(size)
			make.height.equalTo(size)
		}
		return self
	}
	
	func size(w:CGFloat,h:CGFloat) -> UIView {
		snp.makeConstraints { (make) in
			make.width.equalTo(w)
			make.height.equalTo(h)
		}
		return self
	}
	
	func height(_ h:CGFloat) -> UIView {
		snp.makeConstraints { (make) in
			make.height.equalTo(h)
		}
		return self
	}
	
	func height(_ h:Int) -> UIView {
		return height(CGFloat(h))
	}
	
	func width(_ h:CGFloat) -> UIView {
		snp.makeConstraints { (make) in
			make.width.equalTo(h)
		}
		return self
	}
	
	func width(_ h:Int) -> UIView {
		return width(CGFloat(h))
	}
	
	func maxHeight(_ h:CGFloat) -> UIView {
		snp.makeConstraints { (make) in
			make.height.lessThanOrEqualTo(h)
		}
		return self
	}
	
	func minWidth(_ h:CGFloat) -> UIView {
		snp.makeConstraints { (make) in
			make.width.greaterThanOrEqualTo(h)
		}
		return self
	}
	
	
	func matchParentSideBorders(insetedByDx:CGFloat = 0) -> UIView {
		snp.makeConstraints { (make) in
			make.left.equalToSuperview().offset(insetedByDx)
			make.right.equalToSuperview().offset(-insetedByDx)
		}
		return self
	}
	
	func matchParentDimmensions() -> UIView {
		snp.makeConstraints { (make) in
			make.left.right.top.bottom.equalToSuperview()
		}
		return self
	}
	
	func matchDimentionsOf(view:UIView) -> UIView {
		snp.makeConstraints { (make) in
			make.left.right.top.bottom.equalTo(view)
		}
		return self
	}
	
	func matchParentHeight() -> UIView {
		snp.makeConstraints { (make) in
			make.top.bottom.equalToSuperview()
		}
		return self
	}
	
	func matchParentHeightDividedBy(_ divider : CGFloat) -> UIView {
		snp.makeConstraints { (make) in
			make.height.equalToSuperview().dividedBy(divider)
		}
		return self
	}
	
	func matchParentWidthDividedBy(_ divider : CGFloat) -> UIView {
		snp.makeConstraints { (make) in
			make.width.equalToSuperview().dividedBy(divider)
		}
		return self
	}
	
	func center() -> UIView {
		snp.makeConstraints { (make) in
			make.center.equalToSuperview()
		}
		return self
	}
	
	func alignParentTop(withMargin:CGFloat = 0.0) -> UIView {
		snp.makeConstraints { (make) in
			make.top.equalToSuperview().offset(withMargin)
		}
		return self
	}
	
	func alignParentTop(withMargin:Int ) -> UIView {
		return alignParentTop(withMargin:CGFloat(withMargin))
	}
	
	
	func alignUnder(view:UIView, withMargin:CGFloat = 0.0) -> UIView {
		snp.makeConstraints { (make) in
			make.top.equalTo(view.snp.bottom).offset(withMargin)
		}
		return self
	}
	func alignUnder(view:UIView, withMargin:Int) -> UIView {
		return alignUnder(view: view,withMargin:CGFloat(withMargin))
	}
	
	func matchRightOf(view:UIView, withMargin:CGFloat = 0) -> UIView {
		snp.makeConstraints { (make) in
			make.right.equalTo(view).offset(withMargin)
		}
		return self
	}
	
	func updateAlignUnder(view:UIView, withMargin:CGFloat = 0.0) -> UIView {
		snp.updateConstraints { (make) in
			make.top.equalTo(view.snp.bottom).offset(withMargin)
		}
		return self
	}
	
	func alignParentBottom(withMargin:CGFloat = 0.0) -> UIView {
		snp.makeConstraints { (make) in
			make.bottom.equalToSuperview().offset(-withMargin)
		}
		return self
	}
	
	func alignParentBottom(withMargin:Int) -> UIView {
		return alignParentBottom(withMargin:CGFloat(withMargin))
	}
	
	func alignAbove(view:UIView, withMargin:CGFloat = 0.0) -> UIView {
		snp.makeConstraints { (make) in
			make.bottom.equalTo(view.snp.top).offset(-withMargin)
		}
		return self
	}
	
	func alignAbove(view:UIView, withMargin:Int) -> UIView {
		return alignAbove(view: view,withMargin:CGFloat(withMargin))
	}
	
	func alignBottomWith(otherView:UIView) -> UIView {
		snp.makeConstraints { (make) in
			make.bottom.equalTo(otherView)
		}
		return self
	}
	
	func marginLeft(_ m:CGFloat) -> UIView {
		snp.makeConstraints { (make) in
			make.left.equalToSuperview().offset(m)
		}
		return self
	}
	
	func alignParentLeft(withMargin:CGFloat = 0.0) -> UIView {
		snp.makeConstraints { (make) in
			make.left.equalToSuperview().offset(withMargin)
		}
		return self
	}
	
	func alignParentLeft(withMargin:Int) -> UIView {
		return alignParentLeft(withMargin:CGFloat(withMargin))
	}
	
	func alignParentRight(withMargin:Int = 0) -> UIView {
		snp.makeConstraints { (make) in
			make.right.equalToSuperview().offset(-withMargin)
		}
		return self
	}
	
	func alignParentRight(withMargin:CGFloat) -> UIView {
		return alignParentRight(withMargin:Int(withMargin))
	}

	
	func toRightOf(_ view:UIView, withLeftMargin:Int = 0) -> UIView {
		snp.makeConstraints { (make) in
			make.left.equalTo(view.snp.right).offset(withLeftMargin)
		}
		return self
	}
	
	func toRightOf(_ view:UIView, withLeftMargin:CGFloat) -> UIView {
		return toRightOf(view,withLeftMargin: Int(withLeftMargin))
	}
	
	func toLeftOf(_ view:UIView) -> UIView {
		snp.makeConstraints { (make) in
			make.right.equalTo(view.snp.left)
		}
		return self
	}
	
	
	func centerX(withDx:Int = 0) -> UIView {
		snp.makeConstraints { (make) in
			make.centerX.equalToSuperview().offset(withDx)
		}
		return self
	}
	
	func centerY(withDy:Int = 0) -> UIView {
		snp.makeConstraints { (make) in
			make.centerY.equalToSuperview().offset(withDy)
		}
		return self
	}
	
	func matchCenterXOf(view:UIView, withDx:Int = 0) -> UIView {
		snp.makeConstraints { (make) in
			make.centerX.equalTo(view).offset(withDx)
		}
		return self
	}
	
	func matchCenterYOf(view:UIView, withDy:Int = 0) -> UIView {
		snp.makeConstraints { (make) in
			make.centerY.equalTo(view).offset(withDy)
		}
		return self
	}
	
	func wrapContentY() -> UIView {
		subviews.first?.snp.makeConstraints({ make in
			make.top.equalToSuperview()
		})
		subviews.last?.snp.makeConstraints({ make in
			make.bottom.equalToSuperview()
		})
		return self
	}
	
	func wrapContentX() -> UIView {
		subviews.first?.snp.makeConstraints({ make in
			make.left.equalToSuperview()
		})
		subviews.last?.snp.makeConstraints({ make in
			make.right.equalToSuperview()
		})
		return self
	}
	
	func done() {
		// to avoid the unused variable warning
	}
	
	// Onclick
	
	class TapGestureRecognizer: UITapGestureRecognizer {
		var action : (()->Void)? = nil
	}
	
	func onClick(action : @escaping ()->Void ){
		let tap = TapGestureRecognizer(target: self , action: #selector(self.handleTap(_:)))
		tap.action = action
		tap.numberOfTapsRequired = 1
		tap.cancelsTouchesInView = false
		
		self.addGestureRecognizer(tap)
		self.isUserInteractionEnabled = true
		
	}
	@objc func handleTap(_ sender: TapGestureRecognizer) {
		sender.action!()
	}
		
	func VIEW<T>( _ desc: UICompositeViewDescription) -> T{
		return PhoneMainView.instance().mainViewController.getCachedController(desc.name) as! T
	}
		
}
