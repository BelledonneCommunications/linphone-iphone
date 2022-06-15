/*
* Copyright (c) 2010-2020 Belledonne Communications SARL.
*
* This file is part of linhome
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


class MutableLiveDataOnChangeClosure<Type>: NSObject {
    let value: (Type?) -> Void
	let onlyOnce: Bool
    init(_ function: @escaping (Type?) -> Void, onlyOnce:Bool = false) {
        value = function
		self.onlyOnce = onlyOnce
    }
}

class MutableLiveData<T> {
    
    private var _value : T? = nil
	private var observers  =  [MutableLiveDataOnChangeClosure<T>] ()
	private var _opposite : MutableLiveData<Bool>? = nil
	
	init(_ initial:T) {
		self.value = initial
	}
	
	init() {
	}
    
    var value : T? {
        get {
            return self._value
        }
        set {
            self._value = newValue
            self.notifyAllObservers(with: newValue)
        }
    }
    
    
	func addObserver(observer: MutableLiveDataOnChangeClosure<T>, andNotifyOnce: Bool = false) {
        observers.append(observer)
		if (andNotifyOnce) {
			notifyValue()
		}
    }
    
	func removeObserver(observer: MutableLiveDataOnChangeClosure<T>) {
        observers = observers.filter({$0 !== observer})
    }
    
    
	func clearObservers() {
		observers.forEach {
			removeObserver(observer: $0)
		}
    }
    
	
    func notifyAllObservers(with newValue: T?) {
        for observer in observers {
			observer.value(newValue)
			if (observer.onlyOnce) {
				removeObserver(observer: observer)
			}
        }
    }
	
	func notifyValue() {
		for observer in observers {
				   observer.value(value)
				   if (observer.onlyOnce) {
					   removeObserver(observer: observer)
				   }
			   }
	}
	
	func observe(onChange : @escaping (T?)->Void) {
		let observer = MutableLiveDataOnChangeClosure<T>({ value in
			onChange(value)
		}, onlyOnce: false)
		addObserver(observer: observer)
	}
	
	func readCurrentAndObserve(onChange : @escaping (T?)->Void) {
		let observer = MutableLiveDataOnChangeClosure<T>({ value in
			onChange(value)
		}, onlyOnce: false)
		addObserver(observer: observer)
		observer.value(value)
	}
	
	func observeAsUniqueObserver (onChange : @escaping (T?)->Void, unique: Bool = false) {
		let observer = MutableLiveDataOnChangeClosure<T>({ value in
			onChange(value)
		}, onlyOnce: false)
		if (unique) {
			clearObservers()
		}
		addObserver(observer: observer)
	}
	
	func observeOnce(onChange : @escaping (T?)->Void) {
		let observer = MutableLiveDataOnChangeClosure<T>({ value in
			onChange(value)
		}, onlyOnce: true)
		addObserver(observer: observer)
	}
	
	func opposite() -> MutableLiveData<Bool>? {
		if (_opposite != nil) {
			return _opposite
		}
		_opposite = MutableLiveData<Bool>(!(value! as! Bool))
		observe { (value) in
			self._opposite!.value = !(value! as! Bool)
		}
		return _opposite
	}
	
	
}
