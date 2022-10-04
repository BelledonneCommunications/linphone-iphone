import XCTest

class ExtraMenuActiveCallActionsUITestsMethods : ActiveCallViewUITestsMethods {
	
	func displayExtraMenuButtonView() {
		startActiveCall()
		openExtraButtonMenu()
	}
	
	func openCallStatsFromExtraMenuButtonView() {
		app.extraMenuView.buttons["active_call_extra_buttons_stats"].tap()
		
		app.representationWithElements.otherElement = app.callStatsView
		app.representationWithElements.withElementVariations(mainView: ["shadow"], statusBar: ["call_view"], tabBar: []).check()
	}
	
	func closeCallStatsFromItself() {
		app.callStatsView.buttons["call_stats_view_hide"].tap()
		
		app.representationWithElements.reloadBackup(named: "extra_menu_closed").check()
	}
	
	func openCallNumpad() {
		app.extraMenuView.buttons["active_call_extra_buttons_numpad"].tap()
		
		//app.numpadCallView.representation.reMake()
		app.representationWithElements.otherElement = app.numpadCallView
		app.representationWithElements.withElementVariations(mainView: ["shadow"], statusBar: ["call_view"], tabBar: []).check()
	}
	
	func closeCallNumpad() {
		app.numpadCallView.buttons["call_numpad_view_hide"].tap()
		
		app.representationWithElements.reloadBackup(named: "extra_menu_closed").check()
	}
	
	func composeNumpadNumbers() {
		
		let textField = app.staticTexts["call_numpad_view_text_field"]
		var digitsLabel = ["1","2","3","4","5","6","7","8","9","*","0","#"]
		digitsLabel += digitsLabel
		digitsLabel.shuffle()
		for label in digitsLabel {
			app.numpadCallView.buttons["call_numpad_view_digit_\(label)"].tap()

		}
	
		XCTAssertEqual(textField.label, digitsLabel.joined(), "Text Field value differs from the sequence typed (is equal to \"\(textField.label)\")")
	}

	/*
    func openView(buttonTag: String, view: View) {
		let button = app.extraMenuView.buttons["active_call_extra_buttons_\(buttonTag)"].tap()
        //button.tap(action: .displayView, on: view)
    }

    func closeView(contextView: View) {
        let hide = UIObject(identifier: "\(contextView.rawValue)_hide", type: .button, contextView: contextView)
        //hide.tap(action: .hideView, on: contextView)
    }
    
    func backToCall(contextView: View) {
        let button = UIObject(identifier: "back_to_call", type: .button, contextView: contextView)
        //button.tap(action: .displayView, on: .ActiveCallView)
    }
    
    func checkNumpadView() {
        
        UIObject(identifier: "call_numpad_view_hide", type: .button, contextView: .NumpadView)
        let textField = UIObject(identifier: "call_numpad_view_text_field", type: .staticText, contextView: .NumpadView).element

        var digitsLabel = ["1","2","3","4","5","6","7","8","9","*","0","#"]
        digitsLabel += digitsLabel
        digitsLabel.shuffle()
        for label in digitsLabel {
            UIObject(identifier: "call_numpad_view_digit_\(label)", type: .button, contextView: .NumpadView).element.tap()
        }
    
        XCTAssertEqual(textField.label, digitsLabel.joined(), "Text Field value differs from the sequence typed (is equal to \"\(textField.label)\")")
    }
    
    func closeCallsList() {
        let hide = UIObject(identifier: "dismissable_view_close", type: .button, contextView: .CallsListView)
        //hide.tap(action: .hideView, on: .CallsListView)
    }*/
}

