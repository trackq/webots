// Copyright 1996-2019 Cyberbotics Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "WbTriangleMeshCache.hpp"

#include "WbCoordinate.hpp"
#include "WbIndexedFaceSet.hpp"
#include "WbMFInt.hpp"
#include "WbNormal.hpp"
#include "WbSFBool.hpp"
#include "WbSFDouble.hpp"
#include "WbTextureCoordinate.hpp"
#include "WbTriangleMesh.hpp"

#include <cassert>
#include <cstdlib>
#include <functional>

namespace WbTriangleMeshCache {
  const highwayhash::HH_U64 SIPHASH_KEY[2] = {
    0x4242424242424242ull,
    0x4242424242424242ull,
  };

  uint64_t sipHash13c(const char *bytes, const int size) { return highwayhash::SipHash13(SIPHASH_KEY, bytes, size); }
  TriangleMeshInfo::TriangleMeshInfo() : mTriangleMesh(NULL), mNumUsers(0) {}
  TriangleMeshInfo::TriangleMeshInfo(WbTriangleMesh *triangleMesh) : mTriangleMesh(triangleMesh), mNumUsers(1) {}

  IndexedFaceSetKey::IndexedFaceSetKey() { mHash = 0; }
  IndexedFaceSetKey::IndexedFaceSetKey(WbIndexedFaceSet *indexedFaceSet) { set(indexedFaceSet); }

  void IndexedFaceSetKey::set(WbIndexedFaceSet *indexedFaceSet) {
    mHash = 0;

    const WbCoordinate *coord = indexedFaceSet->coord();
    if (coord && coord->pointSize()) {
      const double *startCoord = coord->point().item(0).ptr();
      int size = 3 * coord->pointSize();
      mHash ^= sipHash13x(startCoord, size);
    }

    const WbNormal *normal = indexedFaceSet->normal();
    if (normal && normal->vectorSize()) {
      const double *startNormal = normal->vector().item(0).ptr();
      int size = 2 * normal->vectorSize();
      mHash ^= sipHash13x(startNormal, size);
    }

    const WbTextureCoordinate *texCoord = indexedFaceSet->texCoord();
    if (texCoord && texCoord->pointSize()) {
      const double *startTexCoord = texCoord->point().item(0).ptr();
      int size = 2 * texCoord->pointSize();
      mHash ^= sipHash13x(startTexCoord, size);
    }

    const WbMFInt *coordIndex = indexedFaceSet->coordIndex();
    if (coordIndex && coordIndex->size()) {
      const int *startCoordIndex = &(coordIndex->item(0));
      mHash ^= sipHash13x(startCoordIndex, coordIndex->size());
    }

    const WbMFInt *normalIndex = indexedFaceSet->normalIndex();
    if (normalIndex && normalIndex->size()) {
      const int *startNormalIndex = &(normalIndex->item(0));
      mHash ^= sipHash13x(startNormalIndex, normalIndex->size());
    }

    const WbMFInt *texCoordIndex = indexedFaceSet->texCoordIndex();
    if (texCoordIndex && texCoordIndex->size()) {
      const int *startTexCoordIndex = &(texCoordIndex->item(0));
      mHash ^= sipHash13x(startTexCoordIndex, texCoordIndex->size());
    }

    mHash ^= sipHash13x(indexedFaceSet->creaseAngle()->valuePointer(), 1);
    mHash ^= sipHash13x(indexedFaceSet->ccw()->valuePointer(), 1);
    mHash ^= sipHash13x(indexedFaceSet->normalPerVertex()->valuePointer(), 1);
  }

  bool IndexedFaceSetKey::operator==(const IndexedFaceSetKey &rhs) const { return mHash == rhs.mHash; }

  std::size_t IndexedFaceSetKeyHasher::operator()(const IndexedFaceSetKey &k) const {
    assert(sizeof(size_t) == sizeof(uint64_t));
    return static_cast<size_t>(k.mHash);
  }

  void useTriangleMesh(WbIndexedFaceSet *user) {
    if (user->getTriangleMeshMap().count(user->getMeshKey()) == 0)
      user->getTriangleMeshMap()[user->getMeshKey()] = user->createTriangleMesh();
    else
      ++user->getTriangleMeshMap().at(user->getMeshKey()).mNumUsers;

    user->setTriangleMesh(user->getTriangleMeshMap().at(user->getMeshKey()).mTriangleMesh);
    user->updateOdeData();
  }

  void releaseTriangleMesh(WbIndexedFaceSet *user) {
    TriangleMeshInfo &triangleMeshInfo = user->getTriangleMeshMap().at(user->getMeshKey());
    if (--triangleMeshInfo.mNumUsers == 0) {
      delete triangleMeshInfo.mTriangleMesh;
      user->getTriangleMeshMap().erase(user->getMeshKey());
    } else
      assert(triangleMeshInfo.mNumUsers >= 0);

    user->setTriangleMesh(NULL);
  }
}  // namespace WbTriangleMeshCache
