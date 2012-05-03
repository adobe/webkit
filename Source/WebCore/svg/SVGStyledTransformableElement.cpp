/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(SVG)
#include "SVGStyledTransformableElement.h"

#include "AffineTransform.h"
#include "Attribute.h"
#include "RenderSVGPath.h"
#include "RenderSVGResource.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGPathData.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_TRANSFORM_LIST(SVGStyledTransformableElement, SVGNames::transformAttr, Transform, transform)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGStyledTransformableElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(transform)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGStyledLocatableElement)
END_REGISTER_ANIMATED_PROPERTIES

SVGStyledTransformableElement::SVGStyledTransformableElement(const QualifiedName& tagName, Document* document, ConstructionType constructionType)
    : SVGStyledLocatableElement(tagName, document, constructionType)
{
    registerAnimatedPropertiesForSVGStyledTransformableElement();
}

SVGStyledTransformableElement::~SVGStyledTransformableElement()
{
}

AffineTransform SVGStyledTransformableElement::getCTM(StyleUpdateStrategy styleUpdateStrategy)
{
    return SVGLocatable::computeCTM(this, SVGLocatable::NearestViewportScope, styleUpdateStrategy);
}

AffineTransform SVGStyledTransformableElement::getScreenCTM(StyleUpdateStrategy styleUpdateStrategy)
{
    return SVGLocatable::computeCTM(this, SVGLocatable::ScreenScope, styleUpdateStrategy);
}

AffineTransform SVGStyledTransformableElement::animatedLocalTransform() const
{
    AffineTransform matrix;
    RenderStyle* style = renderer() ? renderer()->style() : 0;

    // If CSS property was set, use that, otherwise fallback to attribute (if set).
    if (style && style->hasTransform()) {
        // Note: objectBoundingBox is an emptyRect for elements like pattern or clipPath.
        // See the "Object bounding box units" section of http://dev.w3.org/csswg/css3-transforms/
        TransformationMatrix transform;
        style->applyTransform(transform, renderer()->objectBoundingBox());

        // Flatten any 3D transform.
        matrix = transform.toAffineTransform();
    } else
        transform().concatenate(matrix);

    if (m_supplementalTransform)
        return *m_supplementalTransform * matrix;
    return matrix;
}

AffineTransform* SVGStyledTransformableElement::supplementalTransform()
{
    if (!m_supplementalTransform)
        m_supplementalTransform = adoptPtr(new AffineTransform);
    return m_supplementalTransform.get();
}

CSSPropertyID SVGStyledTransformableElement::cssPropertyIdForSVGAttributeName(const QualifiedName& name)
{
    //DEFINE_STATIC_LOCAL(HashMap<AtomicStringImpl*, int>, supportedAttributes, ());
    static HashMap<AtomicStringImpl*, CSSPropertyID>* propertyNameToIdMap = 0;
    if (!propertyNameToIdMap) {
        propertyNameToIdMap = new HashMap<AtomicStringImpl*, CSSPropertyID>;
        propertyNameToIdMap->set(SVGNames::transformAttr.localName().impl(), CSSPropertyWebkitTransform);
    }
    
    return propertyNameToIdMap->get(name.localName().impl());
}

void SVGStyledTransformableElement::collectStyleForAttribute(Attribute* attr, StylePropertySet* style)
{
    CSSPropertyID propertyID = SVGStyledTransformableElement::cssPropertyIdForSVGAttributeName(attr->name());
    if (propertyID > 0) {
        addPropertyToAttributeStyle(style, propertyID, attr->value());
        return;
    }
    
    SVGStyledElement::collectStyleForAttribute(attr, style);
}

bool SVGStyledTransformableElement::isPresentationAttribute(const QualifiedName& name) const
{
    CSSPropertyID propertyID = SVGStyledTransformableElement::cssPropertyIdForSVGAttributeName(name);
    if (propertyID > 0)
        return true;
    return SVGStyledElement::isPresentationAttribute(name);
}

void SVGStyledTransformableElement::svgAttributeChanged(const QualifiedName& attrName)
{
    // Leave this function since we need it to update the SVGTransformLists later.
    
    // TODO: What do we have to do here?
    //SVGElementInstance::InvalidationGuard invalidationGuard(this);

    SVGStyledLocatableElement::svgAttributeChanged(attrName);
    return;
}

SVGElement* SVGStyledTransformableElement::nearestViewportElement() const
{
    return SVGTransformable::nearestViewportElement(this);
}

SVGElement* SVGStyledTransformableElement::farthestViewportElement() const
{
    return SVGTransformable::farthestViewportElement(this);
}

FloatRect SVGStyledTransformableElement::getBBox(StyleUpdateStrategy styleUpdateStrategy)
{
    return SVGTransformable::getBBox(this, styleUpdateStrategy);
}

RenderObject* SVGStyledTransformableElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    // By default, any subclass is expected to do path-based drawing
    return new (arena) RenderSVGPath(this);
}

void SVGStyledTransformableElement::toClipPath(Path& path)
{
    updatePathFromGraphicsElement(this, path);
    // FIXME: How do we know the element has done a layout?
    path.transform(animatedLocalTransform());
}

}

#endif // ENABLE(SVG)
