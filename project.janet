(declare-project
  :name "http-client"
  :license "MIT"
  :author "Paul d'Hubert <paul.dhubert@ya.ru>")

(defn sh [s] (let [f (file/popen s) x (:read f :all)] (:close f) x))
# (def curl-cflags (string/trim (sh "curl-config --cflags")))
(def curl-lflags  (string/split " " (string/trim (sh "curl-config --libs"))))

(declare-native
  :name "curl"
  :cflags [;default-cflags]
  :lflags [;default-lflags ;curl-lflags]
  :source @["src/curl.c"])

(declare-source
  :source @["src/http.janet"])
                
